#include "vm.h"
#include "vm_functions.h"
#include <iostream>

using namespace std;

#define VarArray(name, i) ((name) + "[" + std::to_string(i) + "]")

VM::VM(): ast(NULL), state(VMState::NoState)
{

}

VM::~VM()
{
    if(!this->ast)
        return;

    delete this->ast;
    this->ast = NULL;
}

VMValuePtr VM::execute(const string &file)
{
    this->evaluate(this->readFile(file));

    if(!this->ast || (this->state == VMState::Error))
        return VMValuePtr();

    this->result = this->ast->execute(this);
    return this->result;
}

void VM::evaluate(const string &code)
{
    if(this->ast)
        delete this->ast;

    this->state = VMState::NoState;
    VMUnused(code);
}

void VM::dump(const string &file, const string &astfile)
{
    this->evaluate(this->readFile(file));

    if(!this->ast)
        return;

    this->writeFile(astfile, this->ast->dump());
}

void VM::loadAST(NBlock *ast)
{
    this->ast = ast;
}

VMValuePtr VM::error(const string &msg)
{
    cout << msg << endl;

    this->state = VMState::Error;
    this->declarations.clear();
    return VMValuePtr();
}

void VM::syntaxError(const string &token, unsigned int line)
{
    this->error("Syntax error near '" + token + "' at line " + std::to_string(line));
}

bool VM::checkArguments(NFunction *nfunc, NCall *ncall)
{
    if(nfunc->arguments.size() != ncall->arguments.size())
    {
        this->error("'" + nfunc->name->value + "' " +
                    "expects " + std::to_string(nfunc->arguments.size()) + " arguments, " +
                    std::to_string(ncall->arguments.size()) + " given");

        return false;
    }

    for(size_t i = 0; i < nfunc->arguments.size(); i++)
    {
        NArgument* narg = static_cast<NArgument*>(nfunc->arguments[i]);
        VMValuePtr nargtype = narg->type->execute(this);
        VMValuePtr nparam = ncall->arguments[i]->execute(this);

        if(VMFunctions::is_type_compatible(nargtype, nparam))
            continue;

        this->error("'" + nfunc->name->value + "' " +
                    "argument " + std::to_string(i) + " requires '" + nargtype->type_name() + "' type, " +
                    "'" + nparam->type_name() + "' given");

        return false;
    }

    return true;
}

bool VM::checkArraySize(const VMValuePtr &arraysize)
{
    if(!arraysize->is_integer() || arraysize->is_negative())
    {
        if(!arraysize->is_integer())
            this->error("Expected integer-type, '" + arraysize->type_name() + "' given");
        else
            this->error("Array size must be positive, " + std::to_string(arraysize->si_value) + " given");

        return false;
    }

    return true;
}

Node *VM::getSize(NVariable *nvar)
{
    Node* ndecl = this->declaration(nvar->type);

    if(node_inherits(ndecl, NType))
    {
        NType* ntype = static_cast<NType*>(ndecl);

        if(ntype->size)
            return ntype->size;
    }

    return nvar->size;
}

void VM::onAllocating(const VMValuePtr &vmvalue)
{
    VMUnused(vmvalue);
}

Node *VM::declaration(Node *ndecl)
{
    if(node_inherits(ndecl, NType))
    {
        NType* ntype = static_cast<NType*>(ndecl);

        if(ntype->is_basic)
            return ndecl;

        return this->declaration(ntype->name);
    }
    else if(node_inherits(ndecl, NIdentifier))
        return this->declaration(static_cast<NIdentifier*>(ndecl));

    this->error("'" + node_typename(ndecl) + "' unknown declaration type");
    return NULL;
}

Node *VM::declaration(NIdentifier *nid)
{
    auto itdecl = this->declarations.find(nid->value);

    if(itdecl == this->declarations.end())
    {
        this->error("'" + nid->value + "' is undeclared");
        return NULL;
    }

    return itdecl->second;
}

std::string VM::readFile(const string &file) const
{
    FILE *fp = std::fopen(file.c_str(), "rb");

    string s;
    std::fseek(fp, 0, SEEK_END);

    s.resize(std::ftell(fp));

    std::rewind(fp);
    std::fread(&s[0], 1, s.size(), fp);
    std::fclose(fp);

    return s;
}

void VM::writeFile(const string &file, const string &data) const
{
    FILE *fp = std::fopen(file.c_str(), "wb");
    std::fwrite(data.c_str(), sizeof(string::traits_type), data.size(), fp);
    std::fclose(fp);
}

Node *VM::isDeclared(NIdentifier* nid) const
{
    auto it = this->declarations.find(nid->value);

    if(it == this->declarations.end())
        return NULL;

    return it->second;
}

bool VM::allocArguments(NFunction *nfunc, NCall *ncall)
{
    VMVariables locals; // Build local variable stack

    for(size_t i = 0; i < ncall->arguments.size(); i++)
    {
        NArgument* narg = static_cast<NArgument*>(nfunc->arguments[i]);
        VMValuePtr vmargtype = narg->type->execute(this);
        VMValuePtr vmargvalue = ncall->arguments[i]->execute(this);

        if(!narg->by_reference)
            vmargvalue = std::make_shared<VMValue>(*vmargvalue); // Copy value

        if(!VMFunctions::type_cast(vmargtype, vmargvalue))
        {
            this->error("'" + nfunc->name->value + "' function: " +
                        "cannot convert argument " + std::to_string(i) + " from '" + vmargvalue->type_name() +
                        "' to '" + vmargtype->type_name() + "'");

            return false;
        }

        locals[narg->name->value] = vmargvalue;
    }

    this->scopestack.push_back(locals);
    return true;
}

void VM::bindVariable(NVariable *nvar)
{
    if(this->declarationstack.empty())
    {
        VMVariables& variables = this->scopestack.empty() ? this->globals : this->scopestack.back();

        if(variables.find(nvar->name->value) != variables.end())
        {
            this->error("Shadowing variable '" + nvar->name->value + "'");
            return;
        }

        variables[nvar->name->value] = this->allocVariable(nvar);
    }
    else
        this->declarationstack.back()->m_value.push_back(this->allocVariable(nvar));
}

VMValuePtr VM::allocVariable(NVariable *nvar)
{
    VMValuePtr vmtype = nvar->type->execute(this), vmvalue;
    Node* nsize = this->getSize(nvar);

    if(nsize)
    {
        VMValuePtr vmsize = nsize->execute(this);

        if(!this->checkArraySize(vmsize))
            return VMValuePtr();

        if(vmtype->is_string())
            vmvalue = std::make_shared<VMValue>(VMValue::build_string(vmsize->ui_value));
        else
            vmvalue = std::make_shared<VMValue>(VMValue::build_array(vmsize->ui_value));
    }
    else
        vmvalue = std::make_shared<VMValue>(*vmtype); // Copy type information

    vmvalue->value_typedef = this->declaration(nvar->type);
    vmvalue->value_id = nvar->name->value;

    if(nvar->is_const)
        vmvalue->value_flags |= VMValueFlags::Const;

    if(nvar->is_local)
        vmvalue->value_flags |= VMValueFlags::Local;

    this->initVariable(vmtype, vmvalue, (nvar->value ? nvar->value->execute(this) : NULL));
    return vmvalue;
}

void VM::bindVariables(NVariable *nvar)
{
    this->bindVariable(nvar);

    std::for_each(nvar->names.begin(), nvar->names.end(), [this, nvar](Node* n) {
        NVariable* nsubvar = static_cast<NVariable*>(n);

        nsubvar->type = nvar->type; // FIXME: Borrow type
        nsubvar->is_local = nvar->is_local;
        nsubvar->is_const = nvar->is_const;

        this->bindVariable(nsubvar);
        nsubvar->type = NULL;
    });
}

VMValuePtr VM::variable(NIdentifier *id)
{
    VMValuePtr res;

    if(!this->declarationstack.empty() && (res = this->declarationstack.back()->is_member(id->value)))
        return res;

    VMVariables::iterator itvar;

    for(auto it = this->scopestack.rbegin(); it != this->scopestack.rend(); it++) // Loop through parent scopes
    {
        itvar = it->find(id->value);

        if(itvar == it->end())
            continue;

        return itvar->second;
    }

    itvar = this->globals.find(id->value); // Try globals

    if(itvar == this->globals.end())
        return this->error("Undeclared variable '" + id->value + "'");

    return itvar->second;
}

void VM::initVariable(const VMValuePtr& vmtype, const VMValuePtr &vmvalue, const VMValuePtr &defaultvalue)
{
    if(defaultvalue)
    {
        if(!VMFunctions::is_type_compatible(vmtype, defaultvalue))
        {
            this->error("Cannot initialize '" + vmvalue->value_id + "' of type '" + vmvalue->type_name() +
                        "' with '" + defaultvalue->type_name() + "' type");

            return;
        }

        if((vmvalue->is_const() || vmvalue->is_local()))
            vmvalue->assign(*defaultvalue);
    }

    this->onAllocating(vmvalue);

    if(vmvalue->is_array() || node_is_compound(vmvalue->n_value))
        this->declarationstack.push_back(vmvalue);

    if(vmvalue->is_array())
    {
        for(uint64_t i = 0; i < vmvalue->m_value.capacity(); i++)
        {
            VMValuePtr vmelementvalue = std::make_shared<VMValue>(*vmtype); // Copy type information
            vmelementvalue->value_typedef = vmvalue->value_typedef;
            vmelementvalue->value_id = VarArray(vmvalue->value_id, i);

            this->initVariable(vmtype, vmelementvalue);
            vmvalue->m_value.push_back(vmelementvalue);
        }
    }
    else if(node_is(vmvalue->n_value, NEnum))
    {
        NEnum* nenum = static_cast<NEnum*>(vmtype->n_value);
        VMValue vmenumval;

        for(auto it = nenum->members.begin(); it != nenum->members.end(); it++)
        {
            if(!node_is(*it, NEnumValue))
            {
                this->error("Unexpected enum-value of type '" + node_typename(*it) + "'");
                break;
            }

            NEnumValue* nenumval = static_cast<NEnumValue*>(*it);

            if(nenumval->value)
                vmenumval = *nenumval->value->execute(this);
            else
            {
                if(vmenumval.is_null())
                    vmenumval = *nenum->type->execute(this);
                else
                    vmenumval++;
            }

            vmenumval.value_typedef = nenum->type;
            vmenumval.value_id = nenumval->name->value;
            vmvalue->m_value.push_back(std::make_shared<VMValue>(vmenumval));
        }
    }
    else if(node_is_compound(vmvalue->n_value))
    {
        NCompoundType* ncompound = static_cast<NCompoundType*>(vmtype->n_value);

        for(auto it = ncompound->members.begin(); it != ncompound->members.end(); it++)
            (*it)->execute(this);
    }

    if(vmvalue->is_array() || node_is_compound(vmvalue->n_value))
        this->declarationstack.pop_back();
}

void VM::declare(NIdentifier* nid, Node *node)
{
    if(is_anonymous_identifier(nid->value)) // Skip anonymous IDs
        return;

    Node* declnode = this->isDeclared(nid);

    if(declnode)
    {
        this->error("'" + nid->value + "' was declared as '" + node_typename(node) + "'");
        return;
    }

    this->declarations[nid->value] = node;
}

uint64_t VM::sizeOf(Node *node)
{
    if(node_inherits(node, NBasicType))
        return static_cast<NBasicType*>(node)->bits / 8;

    if(node_is(node, NType))
        return this->sizeOf(this->isDeclared(static_cast<NType*>(node)->name));

    if(node_is(node, NVariable))
        return this->sizeOf(static_cast<NVariable*>(node)->type);

    if(node_is(node, NStruct))
        return this->sizeOf(static_cast<NStruct*>(node)->members);

    if(node_is(node, NBlock))
        return this->sizeOf(static_cast<NBlock*>(node)->statements);

    if(node_is(node, NIdentifier))
    {
        NIdentifier* nid = static_cast<NIdentifier*>(node);
        Node* ndecl = this->isDeclared(nid);

        if(ndecl)
            return this->sizeOf(ndecl);

        return this->sizeOf(this->variable(nid));
    }

    if(node_is(node, NEnum))
    {
        NEnum* nenum = static_cast<NEnum*>(node);

        if(nenum->type)
            return this->sizeOf(nenum->type);

        return 4; // 32 Bit signed integer size
    }

    this->error("Cannot get size of type '" + node_typename(node) + "'");
    return 0;
}

uint64_t VM::sizeOf(const NodeList &nodes)
{
    uint64_t size = 0;

    for(auto it = nodes.begin(); it != nodes.end(); it++)
        size += this->sizeOf(*it);

    return size;
}

uint64_t VM::sizeOf(const VMValuePtr &vmvalue)
{
    if(vmvalue->is_string())
        return (vmvalue->s_value.empty() ? 0 : vmvalue->s_value.size() - 1);

    if(vmvalue->is_array())
    {
        if(!vmvalue->m_value.capacity())
        {
            this->error("Cannot calculate the size of '" + vmvalue->value_id + "' (size = 0)");
            return 0;
        }

        if(node_is(vmvalue->value_typedef, NStruct))
            return this->sizeOf(vmvalue->m_value.front()) * vmvalue->m_value.capacity();

        return this->sizeOf(vmvalue->value_typedef) * vmvalue->m_value.capacity();
    }

    if(node_is_compound(vmvalue->n_value))
    {
        if(node_is(vmvalue->n_value, NStruct))
            return this->sizeOf(vmvalue->m_value);

        return this->sizeOf(vmvalue->n_value);
    }

    if(vmvalue->value_typedef)
        return this->sizeOf(vmvalue->value_typedef);

    switch(vmvalue->value_type)
    {
        case VMValueType::u8:
        case VMValueType::s8:
        case VMValueType::Bool:
            return 1;

        case VMValueType::u16:
        case VMValueType::s16:
            return 2;

        case VMValueType::u32:
        case VMValueType::s32:
        case VMValueType::Float:
            return 4;

        case VMValueType::u64:
        case VMValueType::s64:
        case VMValueType::Double:
            return 8;

        default:
            break;
    }

    this->error("Cannot get size of value '" + vmvalue->type_name() + "'");
    return 0;
}

uint64_t VM::sizeOf(const VMValueMembers &vmvmembers)
{
    uint64_t size = 0;

    for(auto it = vmvmembers.begin(); it != vmvmembers.end(); it++)
        size += this->sizeOf(*it);

    return size;
}

bool VM::isVMFunction(NIdentifier *id) const
{
    return this->functions.find(id->value) != this->functions.cend();
}

void VM::call(NCall *ncall)
{
    if(this->isVMFunction(ncall->name))
    {
        this->callVM(ncall);
        return;
    }

    Node* ndecl = this->declaration(ncall->name);

    if(!ndecl)
        return;

    if(!node_is(ndecl, NFunction))
    {
        this->error("Trying to call a '" + node_typename(ndecl) + "' type");
        return;
    }

    NFunction* nfunc = static_cast<NFunction*>(ndecl);

    if(!this->checkArguments(nfunc, ncall) || !this->allocArguments(nfunc, ncall))
        return;

    nfunc->body->execute(this);
    this->scopestack.pop_back();
    this->state = VMState::NoState; // Reset VM's state
}

void VM::callVM(NCall *ncall)
{
    VMFunction vmfunction = this->functions[ncall->name->value];
    this->result = vmfunction(this, ncall);
}
