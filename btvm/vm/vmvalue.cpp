#include "vmvalue.h"
#include <cstring>

VMValue VMValue::build(uint64_t bits, bool issigned, bool isfp)
{
    VMValue btv;

    if(!isfp)
    {
        if(bits == 1)
            btv.value_type = VMValueType::Bool;
        else if(bits <= 8)
            btv.value_type = issigned ? VMValueType::s8 : VMValueType::u8;
        else if(bits <= 16)
            btv.value_type = issigned ? VMValueType::s16 : VMValueType::u16;
        else if(bits <= 32)
            btv.value_type = issigned ? VMValueType::s32 : VMValueType::u32;
        else if(bits <= 64)
            btv.value_type = issigned ? VMValueType::s64 : VMValueType::u64;
    }
    else
        btv.value_type = ((bits < 64) ? VMValueType::Float : VMValueType::Double);

    return btv;
}

VMValue VMValue::build_string(uint64_t size)
{
    return VMValue(VMString(size + 1, '\0'));
}

VMValue VMValue::build_string(const std::string &s)
{
    VMValue vmvalue = VMValue::build_string(s.size());
    std::copy(s.begin(), s.end(), vmvalue.s_value.begin());
    return vmvalue;
}

VMValue VMValue::build_string_reference(VMValue *from, const VMValue &index)
{
    VMValue vmvalue;
    vmvalue.value_flags = from->value_flags | VMValueFlags::StringReference;
    vmvalue.value_type = VMValueType::s8;
    vmvalue.cr_value = &from->s_value[index.ui_value];
    return vmvalue;
}

VMValue VMValue::build_default_integer()
{
    return VMValue::build(32, true, false);
}

VMValue VMValue::build_array(uint64_t size)
{
    VMValue vmvalue(VMValueType::Array);
    vmvalue.m_value.reserve(size);
    return vmvalue;
}

void VMValue::guess_size(bool issigned)
{
    if(ui_value <= 0xFF)
        value_type = issigned ? VMValueType::s8 : VMValueType::u8;
    else if(ui_value <= 0xFFFF)
        value_type = issigned ? VMValueType::s16 : VMValueType::u16;
    else if(ui_value <= 0xFFFFFFFF)
        value_type = issigned ? VMValueType::s32 : VMValueType::u32;
    else
        value_type = issigned ? VMValueType::s64 : VMValueType::u64;
}

bool VMValue::is_const() const     { return (value_flags & VMValueFlags::Const); }
bool VMValue::is_local() const     { return (value_flags & VMValueFlags::Local); }
bool VMValue::is_reference() const { return (value_flags & VMValueFlags::StringReference); }

bool VMValue::is_null() const           { return (value_type == VMValueType::Null); }
bool VMValue::is_string() const         { return (value_type == VMValueType::String); }
bool VMValue::is_array() const          { return (value_type == VMValueType::Array); }
bool VMValue::is_node() const           { return (value_type == VMValueType::Node); }
bool VMValue::is_integer() const        { return (value_type >= VMValueType::Bool) && (value_type <= VMValueType::Double); }
bool VMValue::is_floating_point() const { return (value_type >= VMValueType::Float) && (value_type <= VMValueType::Double); }
bool VMValue::is_scalar() const         { return is_integer() || is_floating_point(); }

bool VMValue::is_negative() const
{
    if(is_integer())
        return si_value < 0;

    if(is_floating_point())
        return d_value < 0;

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

std::shared_ptr<VMValue> VMValue::is_member(const std::string &member) const
{
    for(auto it = m_value.begin(); it != m_value.end(); it++)
    {
        if((*it)->value_id == member)
            return *it;
    }

    return NULL;
}

std::string VMValue::type_name() const
{
    if(value_type == VMValueType::Void)
        return "void";
    else if(value_type == VMValueType::Node)
        return "node";
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

const char *VMValue::to_string() const
{
    if(!is_string())
        throw std::runtime_error("Trying to converting a '" + type_name() + "' to 'string'");

    return s_value.data();
}

void VMValue::assign(const VMValue& rhs)
{
    if(is_reference())
        *cr_value = static_cast<char>(rhs.si_value);

    n_value = rhs.n_value;
    m_value = rhs.m_value;
    s_value = rhs.s_value;

    if(rhs.is_signed())
        si_value = rhs.si_value;
    else
        ui_value = rhs.ui_value;
}

VMValue::operator bool() const
{
    if(is_scalar())
        return ui_value != 0;

    return false;
}

bool VMValue::operator ==(const VMValue& rhs) const
{
    if(is_string())
        return !std::strcmp(s_value.data(), rhs.s_value.data());

    return ui_value == rhs.ui_value;
}

bool VMValue::operator !=(const VMValue& rhs) const
{
    if(is_string())
        return std::strcmp(s_value.data(), rhs.s_value.data());

    return ui_value != rhs.ui_value;
}

bool VMValue::operator <=(const VMValue& rhs) const
{
    if(is_signed() || rhs.is_signed())
        return si_value <= rhs.si_value;

    return ui_value <= rhs.ui_value;
}

bool VMValue::operator >=(const VMValue& rhs) const
{
    if(is_signed() || rhs.is_signed())
        return si_value >= rhs.si_value;

    return ui_value >= rhs.ui_value;
}

bool VMValue::operator <(const VMValue& rhs) const
{
    if(is_signed() || rhs.is_signed())
        return si_value < rhs.si_value;

    return ui_value < rhs.ui_value;
}

bool VMValue::operator >(const VMValue& rhs) const
{
    if(is_signed() || rhs.is_signed())
        return si_value > rhs.si_value;

    return ui_value > rhs.ui_value;
}

VMValue& VMValue::operator ++()
{
    if(is_floating_point())
        d_value++;
    else
        si_value++;

    return *this;
}

VMValue VMValue::operator ++(int)
{
    VMValue btv = *this;
    ++*this;
    return btv;
}

VMValue& VMValue::operator --()
{
    if(is_floating_point())
        d_value--;
    else
        si_value--;

    return *this;
}

VMValue VMValue::operator --(int)
{
    VMValue btv = *this;
    --*this;
    return btv;
}

VMValue VMValue::operator !() const
{
    VMValue btv = *this;
    btv.ui_value = !btv.ui_value;
    return btv;
}

VMValue VMValue::operator ~() const
{
    VMValue btv = *this;

    if((value_type == VMValueType::s8) || (value_type == VMValueType::u8))
        btv.ui_value = ~static_cast<uint8_t>(btv.ui_value);
    else if((value_type == VMValueType::s16) || (value_type == VMValueType::u16))
        btv.ui_value = ~static_cast<uint16_t>(btv.ui_value);
    else if((value_type == VMValueType::s32) || (value_type == VMValueType::u32))
        btv.ui_value = ~static_cast<uint32_t>(btv.ui_value);
    else
        btv.ui_value = ~btv.ui_value;

    return btv;
}

VMValue VMValue::operator +(const VMValue& rhs) const
{
    if(is_string())
    {
        VMString vms;
        vms.reserve(s_value.size() + rhs.s_value.size());
        vms.insert(vms.end(), s_value.begin(), s_value.end());
        vms.insert(vms.end(), rhs.s_value.begin(), rhs.s_value.end());
        return VMValue(vms);
    }

    if(is_floating_point())
        return VMValue(d_value + rhs.ui_value);
    else if(rhs.is_floating_point())
        return VMValue(si_value + rhs.d_value);

    return VMValue(ui_value + rhs.ui_value);
}

VMValue VMValue::operator -(const VMValue& rhs) const
{
    if(is_floating_point())
        return VMValue(d_value - rhs.si_value);
    else if(rhs.is_floating_point())
        return VMValue(si_value - rhs.d_value);

    return VMValue(si_value - rhs.si_value);
}

VMValue VMValue::operator *(const VMValue& rhs) const
{
    if(is_floating_point())
        return VMValue(d_value * rhs.si_value);
    else if(rhs.is_floating_point())
        return VMValue(si_value * rhs.d_value);

    return VMValue(si_value * rhs.si_value);
}

VMValue VMValue::operator /(const VMValue& rhs) const
{
    if(is_floating_point())
        return VMValue(d_value / rhs.si_value);
    else if(rhs.is_floating_point())
        return VMValue(si_value / rhs.d_value);

    return VMValue(si_value / rhs.si_value);
}

VMValue VMValue::operator %(const VMValue& rhs) const
{
    return VMValue(si_value % rhs.si_value);
}

VMValue VMValue::operator &(const VMValue& rhs) const
{
    return VMValue(ui_value & rhs.ui_value);
}

VMValue VMValue::operator |(const VMValue& rhs) const
{
    return VMValue(ui_value | rhs.ui_value);
}

VMValue VMValue::operator ^(const VMValue& rhs) const
{
    return VMValue(ui_value ^ rhs.ui_value);
}

VMValue VMValue::operator <<(const VMValue& rhs) const
{
    return VMValue(ui_value << rhs.ui_value);
}

VMValue VMValue::operator >>(const VMValue& rhs) const
{
    return VMValue(ui_value >> rhs.ui_value);
}

std::shared_ptr<VMValue> VMValue::operator[](const VMValue& index)
{
    if(is_string())
        return std::make_shared<VMValue>(VMValue::build_string_reference(this, index));

    return m_value[index.ui_value];
}

std::shared_ptr<VMValue> VMValue::operator[](const std::string &member)
{
    for(auto it = m_value.begin(); it != m_value.end(); it++)
    {
        if((*it)->value_id == member)
            return *it;
    }

    return std::shared_ptr<VMValue>();
}
