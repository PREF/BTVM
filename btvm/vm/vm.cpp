#include "vm.h"
#include <iostream>

using namespace std;

#define btv_eq_op() { btv = lbtv; \
                      btv->assign(*rbtv); }

#define btv_eq_lr_op(op) { btv = lbtv; \
                           btv->assign(*btv op *rbtv); }

VM::VM(): _ast(NULL), state(VMState::NoState)
{

}

VM::~VM()
{
    if(!this->_ast)
        return;

    delete this->_ast;
    this->_ast = NULL;
}

VMValuePtr VM::execute(const string &file)
{
    this->evaluate(this->readFile(file));

    if(!this->_ast || (this->state == VMState::Error))
        return VMValuePtr();

    return this->interpret(this->_ast);
}

void VM::evaluate(const string &code)
{
    if(this->_ast)
        delete this->_ast;

    this->state = VMState::NoState;
    VMUnused(code);
}

void VM::dump(const string &file, const string &astfile)
{
    this->evaluate(this->readFile(file));

    if(!this->_ast)
        return;

    this->writeFile(astfile, dump_ast(this->_ast));
}

void VM::loadAST(NBlock *ast)
{
    this->_ast = ast;
}

VMValuePtr VM::interpret(const NodeList &nodelist)
{
    VMValuePtr res;

    for(auto it = nodelist.begin(); it != nodelist.end(); it++)
    {
        res = this->interpret(*it);

        if(this->state == VMState::Error)
            return VMValuePtr();
        if(this->state == VMState::Return)
            break;
    }

    return res;
}

VMValuePtr VM::interpret(NConditional *nconditional)
{
    ScopeContext(this);

    if(*this->interpret(nconditional->condition))
        return this->interpret(nconditional->true_block);
    else if(nconditional->false_block)
        return this->interpret(nconditional->false_block);

    return VMValuePtr();
}

VMValuePtr VM::interpret(NDoWhile *ndowhile)
{
    ScopeContext(this);
    VMValuePtr vmvalue;

    do
    {
        vmvalue = this->interpret(ndowhile->true_block);

        int vms = VMFunctions::state_check(&this->state);

        if(vms == VMState::Continue)
            continue;
        if(vms == VMState::Break)
            break;
        if(vms == VMState::Return)
            return vmvalue;
    }
    while(*this->interpret(ndowhile->condition));

    return VMValuePtr();
}

VMValuePtr VM::interpret(NWhile *nwhile)
{
    ScopeContext(this);
    VMValuePtr vmvalue;

    while(*this->interpret(nwhile->condition))
    {
        vmvalue = this->interpret(nwhile->true_block);

        int vms = VMFunctions::state_check(&this->state);

        if(vms == VMState::Continue)
            continue;
        if(vms == VMState::Break)
            break;
        if(vms == VMState::Return)
            return vmvalue;
    }

    return VMValuePtr();
}

VMValuePtr VM::interpret(NFor *nfor)
{
    ScopeContext(this);
    VMValuePtr vmvalue;

    this->interpret(nfor->counter);

    while(*this->interpret(nfor->condition))
    {
        this->interpret(nfor->true_block);
        this->interpret(nfor->update);

        int vms = VMFunctions::state_check(&this->state);

        if(vms == VMState::Continue)
            continue;
        if(vms == VMState::Break)
            break;
        if(vms == VMState::Return)
            return vmvalue;
    }

    return VMValuePtr();

}

VMValuePtr VM::interpret(NSwitch *nswitch)
{
    VMCaseMap casemap = this->buildCaseMap(nswitch);
    auto itcond = casemap.find(*this->interpret(nswitch->expression));

    if(itcond == casemap.end())
    {
        if(!nswitch->defaultcase)
            return VMValuePtr();

        return this->interpret(static_cast<NCase*>(nswitch->defaultcase)->body);
    }

    VMValuePtr vmvalue;

    for(auto it = nswitch->cases.begin() + itcond->second; it != nswitch->cases.end(); it++)
    {
        NCase* ncase = static_cast<NCase*>(*it);
        vmvalue = this->interpret(ncase->body);

        int vms = VMFunctions::state_check(&this->state);

        if(vms == VMState::Break)
            break;
        else if(vms == VMState::Return)
            return vmvalue;
    }

    return VMValuePtr();

}

VMValuePtr VM::interpret(NCompareOperator *ncompare)
{
    if(ncompare->cmp == "==")
        return VMValue::copy_value((*this->interpret(ncompare->left) == *this->interpret(ncompare->right)));
    else if(ncompare->cmp == "!=")
        return VMValue::copy_value((*this->interpret(ncompare->left) != *this->interpret(ncompare->right)));
    else if(ncompare->cmp == "<=")
        return VMValue::copy_value((*this->interpret(ncompare->left) <= *this->interpret(ncompare->right)));
    else if(ncompare->cmp == ">=")
        return VMValue::copy_value((*this->interpret(ncompare->left) >= *this->interpret(ncompare->right)));
    else if(ncompare->cmp == "<")
        return VMValue::copy_value((*this->interpret(ncompare->left) <  *this->interpret(ncompare->right)));
    else if(ncompare->cmp == ">")
        return VMValue::copy_value((*this->interpret(ncompare->left) >  *this->interpret(ncompare->right)));

    return this->error("Unknown conditional operator '" + ncompare->cmp + "'");
}

VMValuePtr VM::interpret(NUnaryOperator *nunary)
{
    VMValuePtr btv = this->interpret(nunary->expression);

    if(!btv->is_scalar())
        return this->error("Cannot use unary operators on '" + btv->type_name() + "' types");

    if(nunary->op == "++")
        return nunary->is_prefix ? VMValue::copy_value(++(*btv)) : VMValue::copy_value((*btv)++);

    if(nunary->op == "--")
        return nunary->is_prefix ? VMValue::copy_value(--(*btv)) : VMValue::copy_value((*btv)--);

    if(nunary->op == "!")
        return VMValue::copy_value(!(*btv));

    if(nunary->op == "~")
        return VMValue::copy_value(~(*btv));

    if(nunary->op == "-")
        return VMValue::copy_value(-(*btv));

    return this->error("Unknown unary operator '" + nunary->op + "'");
}

VMValuePtr VM::interpret(NBinaryOperator *nbinary)
{
    VMValuePtr lbtv = this->interpret(nbinary->left);
    VMValuePtr rbtv = this->interpret(nbinary->right);

    if(!VMFunctions::is_type_compatible(lbtv, rbtv))
        return this->error("Cannot use '" + nbinary->op + "' operator with '" + lbtv->type_name() + "' and '" + rbtv->type_name() + "'");

    VMValuePtr btv;

    if(nbinary->op == "+")
        btv = VMValue::copy_value(*lbtv + *rbtv);
    else if(nbinary->op == "-")
        btv = VMValue::copy_value(*lbtv - *rbtv);
    else if(nbinary->op == "*")
        btv = VMValue::copy_value(*lbtv * *rbtv);
    else if(nbinary->op == "/")
        btv = VMValue::copy_value(*lbtv / *rbtv);
    else if(nbinary->op == "%")
        btv = VMValue::copy_value(*lbtv % *rbtv);
    else if(nbinary->op == "&")
        btv = VMValue::copy_value(*lbtv & *rbtv);
    else if(nbinary->op == "|")
        btv = VMValue::copy_value(*lbtv | *rbtv);
    else if(nbinary->op == "^")
        btv = VMValue::copy_value(*lbtv ^ *rbtv);
    else if(nbinary->op == "<<")
        btv = VMValue::copy_value(*lbtv << *rbtv);
    else if(nbinary->op == ">>")
        btv = VMValue::copy_value(*lbtv >> *rbtv);
    else if(nbinary->op == "&&")
        btv = VMValue::copy_value(*lbtv && *rbtv);
    else if(nbinary->op == "||")
        btv = VMValue::copy_value(*lbtv || *rbtv);
    else if(nbinary->op == "+=")
        btv_eq_lr_op(+)
    else if(nbinary->op == "-=")
        btv_eq_lr_op(-)
    else if(nbinary->op == "*=")
        btv_eq_lr_op(*)
    else if(nbinary->op == "/=")
        btv_eq_lr_op(/)
    else if(nbinary->op == "^=")
        btv_eq_lr_op(^)
    else if(nbinary->op == "&=")
        btv_eq_lr_op(&)
    else if(nbinary->op == "|=")
        btv_eq_lr_op(|)
    else if(nbinary->op == "<<=")
        btv_eq_lr_op(<<)
    else if(nbinary->op == ">>=")
        btv_eq_lr_op(>>)
    else if(nbinary->op == "=")
        btv_eq_op()
    else
        return this->error("Unknown binary operator '" + nbinary->op + "'");

    return btv;
}

VMValuePtr VM::interpret(NIndexOperator *nindex)
{
    VMValuePtr vmindex = this->interpret(nindex->index);

    if(!vmindex->is_integer())
        return this->error("integer-type expected, '" + vmindex->type_name() + "' given");
    else if(vmindex->is_negative())
        return this->error("Positive integer expected, " + vmindex->to_string() + " given");

    VMValuePtr lhs = this->interpret(nindex->expression);
    return (*lhs)[vmindex->ui_value];
}

VMValuePtr VM::interpret(NDotOperator *ndot)
{
    if(!node_is(ndot->right, NIdentifier))
        return this->error("Expected NIdentifier, '" + node_typename(ndot->right) + "' given");

    VMValuePtr vmvalue = this->interpret(ndot->left);

    if(!vmvalue->is_compound())
        return this->error("Cannot use '.' operator on '" + vmvalue->type_name() + "' type");

    NIdentifier* nid = static_cast<NIdentifier*>(ndot->right);

    if(node_is_compound(vmvalue->value_typedef))
        return (*vmvalue)[nid->value];

    return this->error("Cannot access '" + nid->value + "' from '" + vmvalue->value_id + "' of type '" + node_typename(vmvalue->value_typedef) + "'");
}

VMValuePtr VM::interpret(NReturn *nreturn)
{
    this->state = VMState::Return;
    return this->interpret(nreturn->block);
}

VMValuePtr VM::interpret(NCast *ncast)
{
    Node* ndecl = this->declaration(ncast->cast);
    VMValuePtr vmvalue = this->interpret(ncast->expression);

    if(!VMFunctions::type_cast(vmvalue, ndecl))
        return this->error("Cannot convert '" + vmvalue->type_name() + "' to '" + node_typename(ndecl) + "'");

    return vmvalue;
}

VMValuePtr VM::interpret(NSizeOf *nsizeof)
{
    return VMValue::allocate_literal(this->sizeOf(nsizeof->expression));
}

VMValuePtr VM::interpret(Node *node)
{
    if(this->state == VMState::Error)
        return VMValuePtr();

    if(node_is(node, NBlock))
        return this->interpret(static_cast<NBlock*>(node)->statements);
    else if(node_inherits(node, NType) || node_is(node, NFunction))
        this->declare(node);
    else if(node_is(node, NVariable))
        this->declareVariables(static_cast<NVariable*>(node));
    else if(node_is(node, NIdentifier))
        return this->variable(static_cast<NIdentifier*>(node));
    else if(node_is(node, NCast))
        return this->interpret(static_cast<NCast*>(node));
    else if(node_is(node, NReturn))
        return this->interpret(static_cast<NReturn*>(node));
    else if(node_is(node, NCompareOperator))
        return this->interpret(static_cast<NCompareOperator*>(node));
    else if(node_is(node, NUnaryOperator))
        return this->interpret(static_cast<NUnaryOperator*>(node));
    else if(node_is(node, NBinaryOperator))
        return this->interpret(static_cast<NBinaryOperator*>(node));
    else if(node_is(node, NIndexOperator))
        return this->interpret(static_cast<NIndexOperator*>(node));
    else if(node_is(node, NDotOperator))
        return this->interpret(static_cast<NDotOperator*>(node));
    else if(node_is(node, NBoolean))
        return VMValue::allocate_literal(static_cast<NBoolean*>(node)->value, node);
    else if(node_is(node, NInteger))
        return VMValue::allocate_literal(static_cast<NInteger*>(node)->value, node);
    else if(node_is(node, NReal))
        return VMValue::allocate_literal(static_cast<NReal*>(node)->value, node);
    else if(node_is(node, NString))
        return VMValue::allocate_literal(static_cast<NString*>(node)->value, node);
    else if(node_is(node, NDoWhile))
        return this->interpret(static_cast<NDoWhile*>(node));
    else if(node_is(node, NWhile))
        return this->interpret(static_cast<NWhile*>(node));
    else if(node_is(node, NFor))
        return this->interpret(static_cast<NFor*>(node));
    else if(node_is(node, NSwitch))
        return this->interpret(static_cast<NSwitch*>(node));
    else if(node_is(node, NConditional))
        return this->interpret(static_cast<NConditional*>(node));
    else if(node_is(node, NSizeOf))
        return this->interpret(static_cast<NSizeOf*>(node));
    else if(node_is(node, NCall))
        return this->call(static_cast<NCall*>(node));
    else if(node_is(node, NVMState))
        this->state = static_cast<NVMState*>(node)->state;
    else
        throw std::runtime_error("Cannot interpret '" + node_typename(node) + "'");

    return VMValuePtr();
}

void VM::declare(Node *node)
{
    NIdentifier* nid = NULL;
    Node* ntype = node;

    if(node_is(node, NTypedef))
    {
        NTypedef* ntypedef = static_cast<NTypedef*>(node);

        if(node_is_compound(ntypedef->type))
            this->declare(ntypedef->type);

        nid = ntypedef->name;
        ntype = ntypedef->type;
    }
    else if(node_inherits(node, NType))
    {
        NType* ntype = static_cast<NType*>(node);

        if(ntype->is_basic)
            throw std::runtime_error("Trying to declare '" + ntype->name->value + "'");

        nid = ntype->name;
    }
    else if(node_is(node, NFunction))
        nid = static_cast<NFunction*>(node)->name;

    if(!nid)
    {
        throw std::runtime_error("Cannot declare '" + node_typename(ntype) + "'");
        return;
    }

    if(is_anonymous_identifier(nid->value)) // Skip anonymous IDs
        return;

    Node* ndecl = this->isDeclared(nid);

    if(ndecl)
    {
        this->error("'" + nid->value + "' was declared as '" + node_typename(ntype) + "'");
        return;
    }

    VMScope& vmscope = this->_scopestack.empty() ? this->_globalscope : this->_scopestack.back();
    vmscope.declarations[nid->value] = ntype;
}

void VM::declareVariables(NVariable *nvar)
{
    this->declareVariable(nvar);

    std::for_each(nvar->names.begin(), nvar->names.end(), [this, nvar](Node* n) {
        NVariable* nsubvar = static_cast<NVariable*>(n);

        nsubvar->type = nvar->type; // FIXME: Borrow type
        nsubvar->is_local = nvar->is_local;
        nsubvar->is_const = nvar->is_const;

        this->declareVariable(nsubvar);
        nsubvar->type = NULL;
    });
}

void VM::declareVariable(NVariable *nvar)
{
    VMValuePtr vmvar = VMValue::allocate(nvar->name->value);

    if(!this->_declarationstack.empty())
    {
        VMValuePtr vmvalue = this->_declarationstack.back();
        vmvalue->m_value.push_back(vmvar);
        this->allocVariable(vmvar, nvar);
        return;
    }

    VMScope& scope = this->_scopestack.empty() ? this->_globalscope : this->_scopestack.back();

    if(scope.variables.find(nvar->name->value) != scope.variables.end())
    {
        this->error("Shadowing variable '" + nvar->name->value + "'");
        return;
    }

    scope.variables[nvar->name->value] = vmvar;
    this->allocVariable(vmvar, nvar);
}

void VM::allocType(const VMValuePtr& vmvar, Node *node, Node *nsize, const NodeList& nconstructor)
{
    Node* ndecl = node_is(node, NType) ? this->declaration(node) : node;

    if(nsize)
    {
        VMValuePtr vmsize = this->interpret(nsize);

        if(!this->isSizeValid(vmsize))
            return;

        if(!node_is(ndecl, NStringType))
        {
            vmvar->allocate_array(vmsize->ui_value, ndecl);

            for(uint64_t i = 0; i < vmsize->ui_value; i++)
            {
                VMValuePtr vmelement = VMValue::allocate();
                vmvar->m_value.push_back(vmelement);
                this->allocType(vmelement, ndecl);
            }
        }
        else
            vmvar->allocate_string(vmsize->ui_value, ndecl);

        return;
    }

    if(node_is(ndecl, NStruct) || node_is(ndecl, NUnion))
    {
        NCompoundType* ncompound = static_cast<NCompoundType*>(ndecl);

        if(node_is(ndecl, NUnion))
            vmvar->allocate_type(VMValueType::Union, ndecl);
        else
            vmvar->allocate_type(VMValueType::Struct, ndecl);

        if(ncompound->arguments.size() != nconstructor.size())
        {
            this->argumentError(ncompound->name, ncompound->arguments, nconstructor);
            return;
        }

        this->_declarationstack.push_back(vmvar);

        if(!this->pushScope(ncompound->name, ncompound->arguments, nconstructor))
            return;

        this->interpret(ncompound->members);

        this->_scopestack.pop_back();
        this->_declarationstack.pop_back();

        if(node_is(ndecl, NUnion))
            this->readValue(NULL, this->sizeOf(vmvar), true); // Seek away from union
    }
    else if(node_is(ndecl, NEnum))
    {
        VMValuePtr vmenumval;
        NEnum* nenum = static_cast<NEnum*>(ndecl);

        vmvar->allocate_type(VMValueType::Enum, ndecl);

        for(auto it = nenum->members.begin(); it != nenum->members.end(); it++)
        {
            if(!node_is(*it, NEnumValue))
            {
                this->error("Unexpected enum-value of type '" + node_typename(*it) + "'");
                return;
            }

            NEnumValue* nenumval = static_cast<NEnumValue*>(*it);

            if(nenumval->value)
                vmenumval = this->interpret(nenumval->value);
            else
            {
                if(!vmenumval)
                {
                    vmenumval = VMValue::allocate();
                    this->allocType(vmenumval, nenum->type);
                }
                else
                    (*vmenumval)++;
            }

            vmenumval->value_typedef = nenum->type;
            vmenumval->value_id = nenumval->name->value;
            vmvar->m_value.push_back(VMValue::copy_value(*vmenumval));
        }
    }
    else if(node_inherits(ndecl, NScalarType))
    {
        NScalarType* nscalar = static_cast<NScalarType*>(ndecl);
        vmvar->allocate_scalar(nscalar->bits, nscalar->is_signed, nscalar->is_fp, ndecl);
    }
    else if(node_is(ndecl, NBooleanType))
        vmvar->allocate_boolean(ndecl);
    else if(node_is(ndecl, NStringType))
        vmvar->allocate_string(0, ndecl);
    else
        throw std::runtime_error("Unknown type: '" + node_typename(ndecl) + "'");
}

void VM::allocVariable(const VMValuePtr& vmvar, NVariable *nvar)
{
    this->allocType(vmvar, nvar->type, this->arraySize(nvar), nvar->constructor);

    if(nvar->bits)
        vmvar->value_bits = *this->interpret(nvar->bits)->value_ref<int64_t>();

    if(nvar->is_const)
        vmvar->value_flags |= VMValueFlags::Const;

    if(nvar->is_local)
        vmvar->value_flags |= VMValueFlags::Local;

    if(!nvar->is_const && !nvar->is_local)
    {
        this->readValue(vmvar, this->_declarationstack.empty() || !this->_declarationstack.back()->is_union());

        if(this->_declarationstack.empty())
            this->processFormat(vmvar);
    }
    else if(nvar->value)
    {
        VMValuePtr vmvalue = this->interpret(nvar->value);

        if(!VMFunctions::is_type_compatible(vmvar, vmvalue))
        {
            this->error("'" + vmvar->value_id + "': cannot assign '" + vmvalue->type_name() + "' to '" + vmvar->type_name() + "'");
            return;
        }

        vmvar->assign(*vmvalue);
    }
}

VMValuePtr VM::variable(NIdentifier *nid)
{
   VMValuePtr vmvalue;

   if(!this->_declarationstack.empty())
       vmvalue = this->_declarationstack.back()->is_member(nid->value);

   if(!vmvalue)
   {
       vmvalue = this->symbol<VMValuePtr>(nid, [](const VMScope& vmscope, NIdentifier* nid) -> VMValuePtr {
           auto it = vmscope.variables.find(nid->value);
           return it != vmscope.variables.end() ? it->second : NULL;
       });
   }

    if(!vmvalue)
        return this->error("Undeclared variable '" + nid->value + "'");

    return vmvalue;
}

Node *VM::declaration(Node *node)
{
    if(node_is(node, NType))
    {
        NType* ntype = static_cast<NType*>(node);

        if(ntype->is_basic)
            return node;

        return this->declaration(ntype->name);
    }
    else if(node_inherits(node, NIdentifier))
    {
        NIdentifier* nid = static_cast<NIdentifier*>(node);
        Node* ndecl = this->isDeclared(nid);

        if(!ndecl)
            this->error("'" + nid->value + "' is undeclared");

        return ndecl;
    }
    else if(node_inherits(node, NType))
        return node;

    this->error("'" + node_typename(node) + "' unknown declaration type");
    return NULL;
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

void VM::readValue(const VMValuePtr &vmvar, bool seek)
{
    if(vmvar->is_array())
    {
        for(auto it = vmvar->m_value.begin(); it != vmvar->m_value.end(); it++)
            this->readValue(*it, seek);
    }
    else if(vmvar->is_readable())
        this->readValue(vmvar, this->sizeOf(vmvar), seek);
}

bool VM::pushScope(NIdentifier* nid, const NodeList& funcargs, const NodeList& callargs)
{
    VMVariables locals; // Build local variable stack

    for(size_t i = 0; i < callargs.size(); i++)
    {
        NArgument* narg = static_cast<NArgument*>(funcargs[i]);
        VMValuePtr vmarg = this->interpret(callargs[i]);

        if(!narg->by_reference)
            vmarg = VMValue::copy_value(*vmarg); // Copy value

        if(!VMFunctions::type_cast(vmarg, this->declaration(narg->type)))
        {
            this->error("'" + nid->value + "': " +
                        "cannot convert argument " + std::to_string(i) + " from '" + vmarg->type_name() +
                        "' to '" + node_typename(narg->type) + "'");

            return false;
        }

        vmarg->value_id = narg->name->value;
        locals[narg->name->value] = vmarg;
    }

    this->_scopestack.push_back(VMScope(locals));
    return true;
}

VM::VMCaseMap VM::buildCaseMap(NSwitch *nswitch)
{
    auto it = this->_switchmap.find(nswitch);

    if(it != this->_switchmap.end())
        return it->second;

    uint64_t i = 0;
    VMCaseMap casemap;

    for(auto it = nswitch->cases.begin(); it != nswitch->cases.end(); it++)
    {
        if(!node_is(*it, NCase))
        {
            this->error("Expected NCase, got '" + node_typename(*it) + "'");
            return VMCaseMap();
        }

        NCase* ncase = static_cast<NCase*>(*it);

        if(!ncase->value)
        {
            nswitch->defaultcase = ncase;
            continue;
        }

        casemap[*this->interpret(ncase->value)] = i++;
    }

    this->_switchmap[nswitch] = casemap;
    return casemap;
}

int64_t VM::getBits(const VMValuePtr &vmvalue)
{
    return vmvalue->value_bits;
}

int64_t VM::getBits(Node *n)
{
    if(!node_is(n, NVariable))
        throw std::runtime_error("Cannot get bit size from '" + node_typename(n) + "'");

    NVariable* nvar = static_cast<NVariable*>(n);

    if(nvar->bits)
        return *this->interpret(nvar->bits)->value_ref<int64_t>();

    return -1;
}

VMValuePtr VM::call(NCall *ncall)
{
    if(this->isVMFunction(ncall->name))
        return this->callVM(ncall);

    Node* ndecl = this->declaration(ncall->name);

    if(!ndecl)
        return this->error("Function '" +  ncall->name->value + "' is not declared");

    if(!node_is(ndecl, NFunction))
        return this->error("Trying to call a '" + node_typename(ndecl) + "' type");

    NFunction* nfunc = static_cast<NFunction*>(ndecl);

    if(nfunc->arguments.size() != ncall->arguments.size())
        return this->argumentError(nfunc->name, ncall->arguments, nfunc->arguments);

    if(!this->pushScope(nfunc->name, nfunc->arguments, ncall->arguments))
        return VMValuePtr();

    VMValuePtr res = this->interpret(nfunc->body);
    this->_scopestack.pop_back();
    this->state = VMState::NoState; // Reset VM's state
    return res;
}


Node *VM::isDeclared(NIdentifier* nid) const
{
    return this->symbol<Node*>(nid, [](const VMScope& vmscope, NIdentifier* nid) -> Node* {
        auto it = vmscope.declarations.find(nid->value);
        return it != vmscope.declarations.end() ? it->second : NULL;
    });
}

bool VM::isVMFunction(NIdentifier *id) const
{
    return this->functions.find(id->value) != this->functions.cend();
}

VMValuePtr VM::error(const string &msg)
{
    cout << msg << endl;

    this->state = VMState::Error;
    this->_globalscope.variables.clear();
    this->_globalscope.declarations.clear();
    this->_scopestack.clear();
    return VMValuePtr();
}

VMValuePtr VM::argumentError(NCall *ncall, size_t expected)
{
    return this->error("'" + ncall->name->value + "' " + "expects " + std::to_string(expected) + " arguments, " +
                                                                      std::to_string(ncall->arguments.size()) + " given");
}

VMValuePtr VM::argumentError(NIdentifier* nid, const NodeList& given, const NodeList& expected)
{
    return this->error("'" + nid->value + "' " + "expects " + std::to_string(expected.size()) + " arguments, " +
                                                              std::to_string(given.size()) + " given");
}

VMValuePtr VM::typeError(Node *n, const string &expected)
{
    return this->error("Expected '" + expected + "', '" + node_typename(n) + "' given");
}

VMValuePtr VM::typeError(const VMValuePtr &vmvalue, const string &expected)
{
    return this->error("Expected '" + expected + "', '" + vmvalue->type_name() + "' given");
}

void VM::syntaxError(const string &token, unsigned int line)
{
    this->error("Syntax error near '" + token + "' at line " + std::to_string(line));
}

bool VM::isSizeValid(const VMValuePtr &vmvalue)
{
    if(!vmvalue->is_integer() || vmvalue->is_negative())
    {
        if(!vmvalue->is_integer())
            this->error("Expected integer-type, '" + vmvalue->type_name() + "' given");
        else
            this->error("Array size must be positive, " + std::to_string(vmvalue->si_value) + " given");

        return false;
    }

    return true;
}

Node *VM::arraySize(NVariable *nvar)
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

int64_t VM::sizeOf(Node *node)
{
    if(node_inherits(node, NBasicType))
        return static_cast<NBasicType*>(node)->bits / PLATFORM_BITS;
    else if(node_is(node, NType))
        return this->sizeOf(this->declaration(node));
    else if(node_is(node, NIdentifier))
        return this->sizeOf(static_cast<NIdentifier*>(node));
    else if(node_is(node, NVariable))
        return this->sizeOf(static_cast<NVariable*>(node));
    else if(node_is(node, NEnum))
        return this->sizeOf(static_cast<NEnum*>(node)->type);
    else if(node_is(node, NBlock))
        return this->sizeOf(static_cast<NBlock*>(node)->statements);
    else if(node_is(node, NStruct))
        return this->compoundSize(static_cast<NStruct*>(node)->members);
    else if(node_is(node, NUnion))
        return this->unionSize(static_cast<NUnion*>(node)->members);

    return this->sizeOf(this->interpret(node));
}

int64_t VM::sizeOf(NVariable *nvar)
{
    if(nvar->size)
    {
        VMValuePtr vmsize = this->interpret(nvar->size);

        if(!this->isSizeValid(vmsize))
            return 0;

        return this->sizeOf(nvar->type) * vmsize->ui_value;
    }

    return this->sizeOf(nvar->type);
}

int64_t VM::sizeOf(const VMValuePtr &vmvalue)
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

        return this->sizeOf(vmvalue->m_value.front()) * vmvalue->m_value.capacity();
    }

    if(node_is_compound(vmvalue->value_typedef))
    {
        if(node_is(vmvalue->value_typedef, NStruct))
            return this->compoundSize(vmvalue->m_value);
        else if(node_is(vmvalue->value_typedef, NUnion))
            return this->unionSize(vmvalue->m_value);

        return this->sizeOf(vmvalue->value_typedef);
    }

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

int64_t VM::sizeOf(NIdentifier *nid)
{
    Node* ndecl = this->isDeclared(nid);

    if(ndecl)
        return this->sizeOf(ndecl);

    return this->sizeOf(this->variable(nid));
}

VMValuePtr VM::callVM(NCall *ncall)
{
    VMFunction vmfunction = this->functions[ncall->name->value];
    return vmfunction(this, ncall);
}
