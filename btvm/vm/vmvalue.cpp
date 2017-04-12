#include "vmvalue.h"
#include "vm_functions.h"
#include <cstring>

#define return_math_op(op) \
    if(is_floating_point()) \
        return *value_ref<double>() op *rhs.value_ref<uint64_t>(); \
    if(rhs.is_floating_point()) \
        return *value_ref<uint64_t>() op *rhs.value_ref<double>(); \
    return *value_ref<uint64_t>() op *rhs.value_ref<uint64_t>();

#define return_cmp_op(op) \
    if(is_signed() || rhs.is_signed()) \
        return *value_ref<int64_t>() op *rhs.value_ref<int64_t>(); \
    return *value_ref<uint64_t>() op *rhs.value_ref<uint64_t>();

VMValue::VMValue()               : value_flags(VMValueFlags::None), value_type(VMValueType::Null),   value_typedef(NULL), value_bgcolor(ColorInvalid), value_fgcolor(ColorInvalid), value_bits(-1), value_offset(0), s_value_ref(NULL), ui_value(0)     { }
VMValue::VMValue(bool value)     : value_flags(VMValueFlags::None), value_type(VMValueType::Bool),   value_typedef(NULL), value_bgcolor(ColorInvalid), value_fgcolor(ColorInvalid), value_bits(-1), value_offset(0), s_value_ref(NULL), ui_value(value) { }
VMValue::VMValue(int64_t value)  : value_flags(VMValueFlags::None), value_type(VMValueType::s64),    value_typedef(NULL), value_bgcolor(ColorInvalid), value_fgcolor(ColorInvalid), value_bits(-1), value_offset(0), s_value_ref(NULL), ui_value(value) { }
VMValue::VMValue(uint64_t value) : value_flags(VMValueFlags::None), value_type(VMValueType::u64),    value_typedef(NULL), value_bgcolor(ColorInvalid), value_fgcolor(ColorInvalid), value_bits(-1), value_offset(0), s_value_ref(NULL), ui_value(value) { }
VMValue::VMValue(double value)   : value_flags(VMValueFlags::None), value_type(VMValueType::Double), value_typedef(NULL), value_bgcolor(ColorInvalid), value_fgcolor(ColorInvalid), value_bits(-1), value_offset(0), s_value_ref(NULL), d_value(value)  { }

VMValuePtr VMValue::allocate(const std::string &id)
{
    VMValuePtr vmvalue = std::make_shared<VMValue>();
    vmvalue->value_id = id;
    return vmvalue;
}

VMValuePtr VMValue::allocate(VMValueType::VMType valuetype, Node *type)
{
    VMValuePtr vmvalue = std::make_shared<VMValue>();
    vmvalue->value_type = valuetype;
    vmvalue->value_typedef = type;
    return vmvalue;
}

VMValuePtr VMValue::allocate(uint64_t bits, bool issigned, bool isfp, Node *type) { return VMValue::allocate(VMFunctions::scalar_type(bits, issigned, isfp), type); }

VMValuePtr VMValue::allocate_literal(bool value, Node *type)
{
    VMValuePtr vmvalue = VMValue::allocate(VMValueType::Bool, type);
    vmvalue->ui_value = value;
    return vmvalue;
}

VMValuePtr VMValue::allocate_literal(int64_t value, Node *type)
{
    VMValuePtr vmvalue = VMValue::allocate(VMFunctions::integer_literal_type(value), type);
    vmvalue->si_value = value;
    return vmvalue;
}

VMValuePtr VMValue::allocate_literal(uint64_t value, Node *type)
{
    VMValuePtr vmvalue = VMValue::allocate(VMFunctions::integer_literal_type(value), type);
    vmvalue->ui_value = value;
    return vmvalue;
}

VMValuePtr VMValue::allocate_literal(double value, Node *type)
{
    VMValuePtr vmvalue = VMValue::allocate(VMValueType::Double, type);
    vmvalue->d_value = value;
    return vmvalue;
}

VMValuePtr VMValue::allocate_literal(const std::string &value, Node *type)
{
    VMValuePtr vmvalue = VMValue::allocate(VMValueType::String, type);
    vmvalue->allocate_string(value, type);
    return vmvalue;
}


void VMValue::allocate_type(VMValueType::VMType valuetype, uint64_t size, Node *type)
{
    value_type = valuetype;
    value_typedef = type;

    if(size > 0)
        s_value.resize(size, 0);
}

void VMValue::allocate_type(VMValueType::VMType valuetype, Node *type) { allocate_type(valuetype, 0, type); }
void VMValue::allocate_scalar(uint64_t bits, bool issigned, bool isfp, Node *type) { allocate_type(VMFunctions::scalar_type(bits, issigned, isfp), type);  }
void VMValue::allocate_boolean(Node *type) { allocate_type(VMValueType::Bool, 1, type);  }

void VMValue::allocate_array(uint64_t size, Node *type)
{
    allocate_type(VMValueType::Array, type);
    m_value.reserve(size);
}

void VMValue::allocate_string(uint64_t size, Node *type) { allocate_type(VMValueType::String, size, type); }

void VMValue::allocate_string(const std::string &s, Node *type)
{
    allocate_string(s.size(), type);
    std::copy(s.begin(), s.end(), s_value.begin());
}

VMValuePtr VMValue::copy_value(const VMValue &vmsrc) { return std::make_shared<VMValue>(vmsrc); }

void VMValue::change_sign()
{
    if(value_type == VMValueType::u8)
        value_type = VMValueType::s8;
    else if(value_type == VMValueType::u16)
        value_type = VMValueType::s16;
    else if(value_type == VMValueType::u32)
        value_type = VMValueType::s32;
    else if(value_type == VMValueType::u64)
        value_type = VMValueType::s64;
    else if(value_type == VMValueType::s8)
        value_type = VMValueType::u8;
    else if(value_type == VMValueType::s16)
        value_type = VMValueType::u16;
    else if(value_type == VMValueType::s32)
        value_type = VMValueType::u32;
    else if(value_type == VMValueType::s64)
        value_type = VMValueType::u64;
}

void VMValue::assign(const VMValue &rhs)
{
    if(is_floating_point())
        *value_ref<double>() = rhs.d_value;
    else if(is_integer())
    {
        if(is_signed())
            *value_ref<int64_t>() = rhs.si_value;
        else
            *value_ref<uint64_t>() = rhs.ui_value;
    }
    else
    {
        if(is_reference())
            std::memcpy(s_value_ref, rhs.s_value.data(), rhs.s_value.size());
        else
            s_value = rhs.s_value;
    }
}

VMValuePtr VMValue::create_reference(uint64_t offset, VMValueType::VMType valuetype) const
{
    VMValuePtr vmvalue = std::make_shared<VMValue>();
    vmvalue->value_flags = value_flags | VMValueFlags::Reference;
    vmvalue->value_type = (valuetype != VMValueType::Null) ? valuetype : value_type;
    vmvalue->value_typedef = value_typedef;
    vmvalue->s_value_ref = const_cast<char*>(value_ref<char>()) + offset;
    return vmvalue;
}

VMValuePtr VMValue::is_member(const std::string &member) const
{
    for(auto it = m_value.begin(); it != m_value.end(); it++)
    {
        if((*it)->value_id == member)
            return *it;
    }

    return NULL;
}

bool VMValue::is_template() const  { return (!is_const() && !is_local()); }
bool VMValue::is_const() const     { return (value_flags & VMValueFlags::Const); }
bool VMValue::is_local() const     { return (value_flags & VMValueFlags::Local); }
bool VMValue::is_reference() const { return (value_flags & VMValueFlags::Reference); }

bool VMValue::is_readable() const       { return (value_type >= VMValueType::String) || (value_type == VMValueType::Enum); }
bool VMValue::is_null() const           { return (value_type == VMValueType::Null); }
bool VMValue::is_string() const         { return (value_type == VMValueType::String); }
bool VMValue::is_boolean() const        { return (value_type == VMValueType::Bool); }
bool VMValue::is_array() const          { return (value_type == VMValueType::Array); }
bool VMValue::is_enum() const           { return (value_type == VMValueType::Enum); }
bool VMValue::is_union() const          { return (value_type == VMValueType::Union); }
bool VMValue::is_struct() const         { return (value_type == VMValueType::Struct); }
bool VMValue::is_compound() const       { return (value_type >= VMValueType::Enum) && (value_type <= VMValueType::Struct); }
bool VMValue::is_integer() const        { return (value_type >= VMValueType::Bool) && (value_type <= VMValueType::s64); }
bool VMValue::is_floating_point() const { return (value_type >= VMValueType::Float) && (value_type <= VMValueType::Double); }
bool VMValue::is_scalar() const         { return is_integer() || is_floating_point(); }

bool VMValue::is_negative() const
{
    if(is_integer())
        return *value_ref<int64_t>() < 0;

    if(is_floating_point())
        return *value_ref<double>() < 0;

    return false;
}

bool VMValue::is_signed() const
{
    switch(value_type)
    {
        case VMValueType::s8:
        case VMValueType::s16:
        case VMValueType::s32:
        case VMValueType::s64:
            return true;

        default:
            break;
    }

    return false;
}

std::string VMValue::type_name() const
{
    if(value_type == VMValueType::Null)
        return "null";
    else if(value_type == VMValueType::Enum)
        return "enum";
    else if(value_type == VMValueType::Union)
        return "union";
    else if(value_type == VMValueType::Struct)
        return "struct";
    else if(value_type == VMValueType::Array)
        return "array";
    else if(value_type == VMValueType::String)
        return "string";
    else if(value_type == VMValueType::Bool)
        return "bool";
    else if(value_type == VMValueType::u8)
        return "u8";
    else if(value_type == VMValueType::u16)
        return "u16";
    else if(value_type == VMValueType::u32)
        return "u32";
    else if(value_type == VMValueType::u64)
        return "u64";
    else if(value_type == VMValueType::s8)
        return "s8";
    else if(value_type == VMValueType::s16)
        return "s16";
    else if(value_type == VMValueType::s32)
        return "s32";
    else if(value_type == VMValueType::s64)
        return "s64";
    else if(value_type == VMValueType::Float)
        return "float";
    else if(value_type == VMValueType::Double)
        return "double";

    return "unknown";
}

std::string VMValue::to_string() const
{
    std::string s = printable(10);

    if(s.empty())
        throw std::runtime_error("Trying to converting a '" + type_name() + "' to string");

    return s;
}

std::string VMValue::printable(int base) const
{
    if(is_integer())
        return VMFunctions::number_to_string((is_signed() ? *value_ref<int64_t>() : *value_ref<uint64_t>()), base);
    else if(is_floating_point())
        return std::to_string(*value_ref<double>());
    else if(is_string())
        return value_ref<char>();

    return std::string();
}

int32_t VMValue::length() const
{
    if(!is_string())
        return 0;

    return std::strlen(value_ref<char>()) + 1;
}

VMValue::operator bool() const
{
    if(is_scalar())
        return (*value_ref<uint64_t>() != 0);

    return false;
}

bool VMValue::operator ==(const VMValue& rhs) const
{
    if(is_string())
        return !std::strcmp(value_ref<char>(), rhs.value_ref<char>());

    return_cmp_op(==);
}

bool VMValue::operator !=(const VMValue& rhs) const
{
    if(is_string())
        return std::strcmp(value_ref<char>(), rhs.value_ref<char>());

    return_cmp_op(!=);
}

bool VMValue::operator <=(const VMValue& rhs) const { return_cmp_op(<=); }
bool VMValue::operator >=(const VMValue& rhs) const { return_cmp_op(>=); }
bool VMValue::operator <(const VMValue& rhs)  const { return_cmp_op(<);  }
bool VMValue::operator >(const VMValue& rhs)  const { return_cmp_op(>);  }

VMValue& VMValue::operator ++()
{
    if(is_floating_point())
        (*value_ref<double>())++;
    else
        (*value_ref<int64_t>())++;

    return *this;
}

VMValue VMValue::operator ++(int)
{
    VMValue vmvalue = *this;
    ++*this;
    return vmvalue;
}

VMValue& VMValue::operator --()
{
    if(is_floating_point())
        (*value_ref<double>())--;
    else
        (*value_ref<int64_t>())--;

    return *this;
}

VMValue VMValue::operator --(int)
{
    VMValue vmvalue = *this;
    --*this;
    return vmvalue;
}

VMValue VMValue::operator !() const { return !(*value_ref<uint64_t>()); }

VMValue VMValue::operator ~() const
{
    if((value_type == VMValueType::s8) || (value_type == VMValueType::u8))
        return static_cast<uint64_t>(~(*value_ref<uint8_t>()));

    if((value_type == VMValueType::s16) || (value_type == VMValueType::u16))
        return static_cast<uint64_t>(~(*value_ref<uint16_t>()));

    if((value_type == VMValueType::s32) || (value_type == VMValueType::u32))
         return static_cast<uint64_t>(~(*value_ref<uint32_t>()));

    return ~(*value_ref<uint64_t>());
}

VMValue VMValue::operator -() const
{
    if(!is_integer())
        throw std::runtime_error("Trying to change sign in a '" + type_name());

    VMValue vmvalue = *this;
    vmvalue.si_value = -vmvalue.si_value;
    vmvalue.change_sign();
    return vmvalue;
}

VMValue VMValue::operator +(const VMValue& rhs) const
{
    if(is_string())
    {
        VMValue vmvalue;
        vmvalue.s_value.resize(s_value.size() + rhs.s_value.size());
        vmvalue.s_value.insert(vmvalue.s_value.end(), s_value.begin(), s_value.end());
        vmvalue.s_value.insert(vmvalue.s_value.end(), rhs.s_value.begin(), rhs.s_value.end());
        return vmvalue;
    }

    return_math_op(+);
}

VMValue VMValue::operator -(const VMValue& rhs) const { return_math_op(-) }
VMValue VMValue::operator *(const VMValue& rhs) const { return_math_op(*); }
VMValue VMValue::operator /(const VMValue& rhs) const { return_math_op(/); }

VMValue VMValue::operator %(const VMValue& rhs)  const { return *value_ref<int64_t>()  %  *rhs.value_ref<int64_t>();  }
VMValue VMValue::operator &(const VMValue& rhs)  const { return *value_ref<uint64_t>() &  *rhs.value_ref<uint64_t>(); }
VMValue VMValue::operator |(const VMValue& rhs)  const { return *value_ref<uint64_t>() |  *rhs.value_ref<uint64_t>(); }
VMValue VMValue::operator ^(const VMValue& rhs)  const { return *value_ref<uint64_t>() ^  *rhs.value_ref<uint64_t>(); }
VMValue VMValue::operator <<(const VMValue& rhs) const { return *value_ref<uint64_t>() << *rhs.value_ref<uint64_t>(); }
VMValue VMValue::operator >>(const VMValue& rhs) const { return *value_ref<uint64_t>() >> *rhs.value_ref<uint64_t>(); }

VMValuePtr VMValue::operator[](const VMValue &index) const
{
    if(is_string())
        return VMValue::create_reference(index.ui_value, VMValueType::s8);

    return m_value[index.ui_value];
}

VMValuePtr VMValue::operator[](const std::string &member) const
{
    for(auto it = m_value.cbegin(); it != m_value.cend(); it++)
    {
        if((*it)->value_id == member)
            return *it;
    }

    return VMValuePtr();
}
