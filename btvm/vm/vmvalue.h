#ifndef VMVALUE_H
#define VMVALUE_H

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <map>

class Node;
class VMValue;

namespace VMValueType
{
    enum VMType { Null = 0,
                  Enum, Union, Struct,
                  Array, String,
                  Bool,
                  u8, u16, u32, u64,
                  s8, s16, s32, s64,
                  Float, Double };
}

namespace VMValueFlags
{
    enum VMFlags { None = 0, Const = 1, Local = 2, Reference = 4 };
}

typedef std::vector<char> VMString;
typedef std::shared_ptr<VMValue> VMValuePtr;
typedef std::vector<VMValuePtr> VMValueMembers;

struct VMValue
{
    VMValue();
    VMValue(bool value);
    VMValue(int64_t value);
    VMValue(uint64_t value);
    VMValue(double value);

    static VMValuePtr allocate(const std::string& id = std::string());
    static VMValuePtr allocate(VMValueType::VMType valuetype, Node* type = NULL);
    static VMValuePtr allocate(uint64_t bits, bool issigned, bool isfp, Node* type = NULL);
    static VMValuePtr allocate_literal(bool value, Node* type = NULL);
    static VMValuePtr allocate_literal(int64_t value, Node* type = NULL);
    static VMValuePtr allocate_literal(uint64_t value, Node* type = NULL);
    static VMValuePtr allocate_literal(double value, Node* type = NULL);
    static VMValuePtr allocate_literal(const std::string& value, Node* type = NULL);
    void allocate_type(VMValueType::VMType valuetype, Node* type);
    void allocate_type(VMValueType::VMType valuetype, uint64_t size, Node* type);
    void allocate_scalar(uint64_t bits, bool issigned, bool isfp, Node* type = NULL);
    void allocate_boolean(Node* type);
    void allocate_array(uint64_t size, Node* type);
    void allocate_string(uint64_t size, Node* type);
    void allocate_string(const std::string& s, Node* type);

    static VMValuePtr copy_value(const VMValue &vmsrc);

    void change_sign();
    void assign(const VMValue& rhs);
    VMValuePtr create_reference(uint64_t offset, VMValueType::VMType valuetype = VMValueType::Null) const;
    VMValuePtr is_member(const std::string& member) const;

    bool is_template() const;
    bool is_const() const;
    bool is_local() const;
    bool is_reference() const;

    bool is_readable() const;
    bool is_null() const;
    bool is_string() const;
    bool is_boolean() const;
    bool is_array() const;
    bool is_enum() const;
    bool is_union() const;
    bool is_struct() const;
    bool is_compound() const;
    bool is_integer() const;
    bool is_floating_point() const;
    bool is_scalar() const;
    bool is_negative() const;
    bool is_signed() const;

    std::string type_name() const;
    std::string to_string() const;
    int32_t length() const;

    operator bool() const;

    bool operator ==(const VMValue& rhs) const;
    bool operator !=(const VMValue& rhs) const;
    bool operator <=(const VMValue& rhs) const;
    bool operator >=(const VMValue& rhs) const;
    bool operator <(const VMValue& rhs) const;
    bool operator >(const VMValue& rhs) const;

    VMValue& operator ++();
    VMValue& operator --();
    VMValue operator ++(int);
    VMValue operator --(int);
    VMValue operator !() const;
    VMValue operator ~() const;
    VMValue operator -() const;
    VMValue operator +(const VMValue& rhs) const;
    VMValue operator -(const VMValue& rhs) const;
    VMValue operator *(const VMValue& rhs) const;
    VMValue operator /(const VMValue& rhs) const;
    VMValue operator %(const VMValue& rhs) const;
    VMValue operator &(const VMValue& rhs) const;
    VMValue operator |(const VMValue& rhs) const;
    VMValue operator ^(const VMValue& rhs) const;
    VMValue operator <<(const VMValue& rhs) const;
    VMValue operator >>(const VMValue& rhs) const;
    VMValuePtr operator[](const VMValue& index) const;
    VMValuePtr operator[](const std::string& member) const;

    template<typename T> const T* value_ref() const;
    template<typename T> T* value_ref();

    size_t               value_flags;
    VMValueType::VMType  value_type;
    Node*                value_typedef;
    std::string          value_id;
    int64_t              value_bits;

    VMValueMembers       m_value;
    VMString             s_value;

    union // By reference
    {
        int64_t*  si_value_ref;
        uint64_t* ui_value_ref;
        double*   d_value_ref;
        char*     s_value_ref;
    };

    union // By value
    {
        int64_t  si_value;
        uint64_t ui_value;
        double   d_value;
    };
};

template<typename T> const T* VMValue::value_ref() const
{
    if(is_integer() || is_enum())
    {
        if(is_signed())
            return reinterpret_cast<const T*>((is_reference() ? si_value_ref : &si_value));

        return reinterpret_cast<const T*>((is_reference() ? ui_value_ref : &ui_value));
    }
    else if(is_floating_point())
        return reinterpret_cast<const T*>((is_reference() ? d_value_ref : &d_value));

    return reinterpret_cast<const T*>((is_reference() ? s_value_ref : s_value.data()));
}

template<typename T> T* VMValue::value_ref() { return const_cast<T*>(static_cast<const VMValue*>(this)->value_ref<T>()); }

struct VMValueHasher
{
    std::size_t operator()(const VMValue& key) const
    {
        if(key.is_integer())
            return std::hash<uint64_t>()(*key.value_ref<uint64_t>());
        else if(key.is_floating_point())
            return std::hash<double>()(*key.value_ref<double>());
        else if(key.is_string())
            return std::hash<std::string>()(key.value_ref<char>());

        throw std::runtime_error("Cannot hash '" + key.type_name() + "'");
        return std::size_t();
    }
};

#endif // VMVALUE_H
