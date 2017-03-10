#include "ast.h"
#include "../vm.h"
#include "../vm_functions.h"

using namespace std;

unsigned long long Node::global_id = 0;

#define build_array_type(vmarray, element) for(size_t i = 0; i < vmarray->m_value.capacity(); i++) \
                                                vmarray->m_value.push_back(std::make_shared<VMValue>(element));

VMValuePtr Node::execute(VM *vm)
{
    VMUnused(vm);
    return VMValuePtr();
}

VMValuePtr NBoolean::execute(VM *vm)
{
    VMUnused(vm);
    return std::make_shared<VMValue>(value);
}

VMValuePtr NInteger::execute(VM *vm)
{
    VMUnused(vm);
    return std::make_shared<VMValue>(value);
}

VMValuePtr NReal::execute(VM *vm)
{
    VMUnused(vm);
    return std::make_shared<VMValue>(value);
}

VMValuePtr NLiteralString::execute(VM *vm)
{
    VMUnused(vm);
    return std::make_shared<VMValue>(VMValue::build_string(value));
}

VMValuePtr NIdentifier::execute(VM *vm)
{
    return vm->variable(this);
}

VMValuePtr NType::execute(VM *vm)
{
    Node* decl = vm->declaration(this);

    if(node_is_compound(decl))
        return std::make_shared<VMValue>(decl);

    return decl->execute(vm);
}

VMValuePtr NBooleanType::execute(VM *vm)
{
    VMUnused(vm);
    return std::make_shared<VMValue>(VMValue::build(1, false, false));
}

VMValuePtr NScalarType::execute(VM *vm)
{
    VMUnused(vm);
    return std::make_shared<VMValue>(VMValue::build(bits, is_signed, is_fp));
}

VMValuePtr NStringType::execute(VM *vm)
{
    VMUnused(vm);
    return std::make_shared<VMValue>(VMValue::build_string());
}

VMValuePtr NEnumValue::execute(VM* vm)
{
    if(value)
        return value->execute(vm);

    return VMValuePtr();
}

VMValuePtr NEnum::execute(VM *vm)
{
    vm->declare(name, this);
    return VMValuePtr();
}

VMValuePtr NBlock::execute(VM *vm)
{
    if(vm->state == VMState::Error)
        return VMValuePtr();

    VMValuePtr btv;

    for(auto it = statements.begin(); it != statements.end(); it++)
    {
        btv = (*it)->execute(vm);

        if(vm->state == VMState::Error)
            return VMValuePtr();

        if(vm->state == VMState::Return)
            break;
    }

    return btv;
}

VMValuePtr NReturn::execute(VM *vm)
{
    vm->state = VMState::Return;
    vm->result = block->execute(vm);
    return vm->result;
}

VMValuePtr NBinaryOperator::execute(VM *vm)
{
    VMValuePtr lbtv = left->execute(vm), rbtv = right->execute(vm);

    if(!VMFunctions::is_type_compatible(lbtv, rbtv))
        return vm->error("Cannot use '" + op + "' operator with '" + lbtv->type_name() + "' and '" + rbtv->type_name() + "'");

    VMValuePtr btv;

    if(op == "+")
        btv = std::make_shared<VMValue>(*lbtv + *rbtv);
    else if(op == "-")
        btv = std::make_shared<VMValue>(*lbtv - *rbtv);
    else if(op == "*")
        btv = std::make_shared<VMValue>(*lbtv * *rbtv);
    else if(op == "/")
        btv = std::make_shared<VMValue>(*lbtv / *rbtv);
    else if(op == "%")
        btv = std::make_shared<VMValue>(*lbtv % *rbtv);
    else if(op == "&")
        btv = std::make_shared<VMValue>(*lbtv & *rbtv);
    else if(op == "|")
        btv = std::make_shared<VMValue>(*lbtv | *rbtv);
    else if(op == "^")
        btv = std::make_shared<VMValue>(*lbtv ^ *rbtv);
    else if(op == "&&")
        btv = std::make_shared<VMValue>(*lbtv && *rbtv);
    else if(op == "||")
        btv = std::make_shared<VMValue>(*lbtv || *rbtv);
    else if(op == "<<")
        btv = std::make_shared<VMValue>(*lbtv << *rbtv);
    else if(op == ">>")
        btv = std::make_shared<VMValue>(*lbtv >> *rbtv);
    else if(op == "=")
    {
        btv = lbtv;
        btv->assign(*rbtv);
    }
    else if(op == "+=")
    {
        btv = lbtv;
        btv->assign(*btv + *rbtv);
    }
    else if(op == "-=")
    {
        btv = lbtv;
        btv->assign(*btv - *rbtv);
    }
    else if(op == "*=")
    {
        btv = lbtv;
        btv->assign(*btv * *rbtv);
    }
    else if(op == "/=")
    {
        btv = lbtv;
        btv->assign(*btv / *rbtv);
    }
    else if(op == "^=")
    {
        btv = lbtv;
        btv->assign(*btv ^ *rbtv);
    }
    else if(op == "&=")
    {
        btv = lbtv;
        btv->assign(*btv & *rbtv);
    }
    else if(op == "|=")
    {
        btv = lbtv;
        btv->assign(*btv | *rbtv);
    }
    else if(op == "<<=")
    {
        btv = lbtv;
        btv->assign(*btv << *rbtv);
    }
    else if(op == ">>=")
    {
        btv = lbtv;
        btv->assign(*btv >> *rbtv);
    }
    else
        return vm->error("Unknown binary operator '" + op + "'");

    return btv;
}

VMValuePtr NUnaryOperator::execute(VM *vm)
{
    VMValuePtr btv = expression->execute(vm);

    if(!btv->is_scalar())
        return vm->error("Cannot use unary operators on '" + btv->type_name() + "' types");

    if(op == "++")
    {
        if(is_prefix)
            btv = std::make_shared<VMValue>(++(*btv));
        else
            btv = std::make_shared<VMValue>((*btv)++);
    }
    else if(op == "--")
    {
        if(is_prefix)
            btv = std::make_shared<VMValue>(--(*btv));
        else
            btv = std::make_shared<VMValue>((*btv)--);
    }
    else if(op == "!")
        return std::make_shared<VMValue>(!(*btv));
    else if(op == "~")
        return std::make_shared<VMValue>(~(*btv));
    else if(op == "-")
        return std::make_shared<VMValue>((*btv) * VMValue(static_cast<int64_t>(-1)));
    else
        return vm->error("Unknown unary operator '" + op + "'");

    return btv;
}

VMValuePtr NIndexOperator::execute(VM *vm)
{
    VMValuePtr vmindex = index->execute(vm);

    if(!vmindex->is_integer())
        return vm->error("integer-type expected, '" + vmindex->type_name() + "' given");

    return (*expression->execute(vm))[*vmindex];
}

VMValuePtr NCompareOperator::execute(VM *vm)
{
    if(cmp == "==")
        return std::make_shared<VMValue>((*left->execute(vm) == *right->execute(vm)));
    else if(cmp == "!=")
        return std::make_shared<VMValue>((*left->execute(vm) != *right->execute(vm)));
    else if(cmp == "<=")
        return std::make_shared<VMValue>((*left->execute(vm) <= *right->execute(vm)));
    else if(cmp == ">=")
        return std::make_shared<VMValue>((*left->execute(vm) >= *right->execute(vm)));
    else if(cmp == "<")
        return std::make_shared<VMValue>((*left->execute(vm) <  *right->execute(vm)));
    else if(cmp == ">")
        return std::make_shared<VMValue>((*left->execute(vm) >  *right->execute(vm)));

    return vm->error("Unknown conditional operator '" + cmp + "'");
}

VMValuePtr NConditional::execute(VM *vm)
{
    if(*condition->execute(vm))
        return if_body->execute(vm);
    else if(body)
        return body->execute(vm);

    return VMValuePtr();
}

VMValuePtr NWhile::execute(VM *vm)
{
    ScopeContext(vm);

    while(*condition->execute(vm))
    {
        body->execute(vm);

        int vms = VMFunctions::state_check(&vm->state);

        if(vms == VMState::Continue)
            continue;
        else if(vms == VMState::Break)
            break;
        else if(vms == VMState::Return)
            return vm->result;
    }

    return VMValuePtr();
}

VMValuePtr NDoWhile::execute(VM *vm)
{
    ScopeContext(vm);

    do
    {
        body->execute(vm);

        int vms = VMFunctions::state_check(&vm->state);

        if(vms == VMState::Continue)
            continue;
        else if(vms == VMState::Break)
            break;
        else if(vms == VMState::Return)
            return vm->result;
    }
    while(*condition->execute(vm));

    return VMValuePtr();
}

VMValuePtr NFor::execute(VM *vm)
{
    ScopeContext(vm);
    counter->execute(vm);

    while(*condition->execute(vm))
    {
        body->execute(vm);
        update->execute(vm);

        int vms = VMFunctions::state_check(&vm->state);

        if(vms == VMState::Continue)
            continue;
        else if(vms == VMState::Break)
            break;
        else if(vms == VMState::Return)
            return vm->result;
    }

    return VMValuePtr();
}

VMValuePtr NSizeOf::execute(VM *vm)
{
    return std::make_shared<VMValue>(vm->sizeOf(expression));
}

VMValuePtr NFunction::execute(VM *vm)
{
    vm->declare(name, this);
    return VMValuePtr();
}

VMValuePtr NCall::execute(VM *vm)
{
    vm->call(this);
    return vm->result;
}

VMValuePtr NCast::execute(VM* vm)
{
    VMValuePtr vmtype = cast->execute(vm);
    VMValuePtr vmvalue = expression->execute(vm);

    if(!VMFunctions::is_type_compatible(vmtype, vmvalue))
        return vm->error("Cannot convert '" + vmtype->type_name() + "' to '" + vmvalue->type_name() + "'");

    if(!VMFunctions::type_cast(vmtype, vmvalue))
        return vm->error("Invalid cast '" + vmtype->type_name() + "' with '" + vmvalue->type_name() + "'");

    return vmvalue;
}

VMValuePtr NVariable::execute(VM *vm)
{
    vm->bindVariables(this);
    return vm->variable(this->name);
}

VMValuePtr NArgument::execute(VM *vm)
{
    VMUnused(vm);
    throw std::runtime_error("Trying to call NArgument::execute()");
    return VMValuePtr();
}

VMValuePtr NCase::execute(VM *vm)
{
    return body->execute(vm);
}

VMValuePtr NSwitch::execute(VM *vm)
{
    if(casemap.empty())
    {
        int i = 0;

        for(auto it = cases.begin(); it != cases.end(); it++)
        {
            NCase* ncase = dynamic_cast<NCase*>(*it);

            if(!ncase->value)
            {
                this->defaultcase = ncase;
                continue;
            }

            this->casemap[*ncase->value->execute(vm)] = i++;
        }
    }

    //ScopeContext(vm);
    VMValue condition = *expression->execute(vm);
    auto itcond = casemap.find(condition);

    if(itcond == casemap.end())
    {
        if(!defaultcase)
            return VMValuePtr();

        return defaultcase->execute(vm);
    }

    for(auto it = cases.begin() + itcond->second; it != cases.end(); it++)
    {
        (*it)->execute(vm);

        int vms = VMFunctions::state_check(&vm->state);

        if(vms == VMState::Break)
            break;
        else if(vms == VMState::Return)
            return vm->result;
    }

    return VMValuePtr();
}

VMValuePtr NCompoundType::execute(VM *vm)
{
    vm->declare(name, this);
    return std::make_shared<VMValue>(this);
}

VMValuePtr NTypedef::execute(VM *vm)
{
    VMValuePtr res;

    if(node_is_compound(type))
        res = type->execute(vm);

    vm->declare(name, type);
    return res;
}

VMValuePtr NVMState::execute(VM *vm)
{
    vm->state = state;
    return VMValuePtr();
}

VMValuePtr NDotOperator::execute(VM* vm)
{
    VMValuePtr vmvl = left->execute(vm);

    if(!vmvl->is_node())
        return vm->error("Cannot use '.' operator on '" + vmvl->type_name() + "' type");

    NIdentifier* nid = static_cast<NIdentifier*>(right);

    if(node_is_compound(vmvl->n_value))
        return (*vmvl)[nid->value];

    return vm->error("Cannot access '" + nid->value + "' from '" + vmvl->value_id + "' of type '" + node_typename(vmvl->n_value) + "'");
}
