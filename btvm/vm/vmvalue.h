#ifndef VMVALUE_H
#define VMVALUE_H

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <map>

class Node;

namespace VMValueType
{
    enum VMType { Null = 0, Void,
                  Node, Array, String, Bool,
                  u8, u16, u32, u64,
                  s8, s16, s32, s64,
                  Float, Double };
}

namespace VMValueFlags
{
    enum VMFlags { None = 0, Const = 1, Local = 2, StringReference = 4 };
}

typedef std::vector<char> VMString;

struct VMValue
{
    VMValue()                                                 : value_flags(VMValueFlags::None), value_type(VMValueType::Null),   value_typedef(NULL), cr_value(NULL), n_value(NULL), ui_value(0) { }
    VMValue(VMValueType::VMType btt)                          : value_flags(VMValueFlags::None), value_type(btt),                 value_typedef(NULL), cr_value(NULL), n_value(NULL), ui_value(0) { }
    VMValue(Node* n)                                          : value_flags(VMValueFlags::None), value_type(VMValueType::Node),   value_typedef(NULL), cr_value(NULL), n_value(n)                 { }
    VMValue(const std::vector< std::shared_ptr<VMValue> >& a) : value_flags(VMValueFlags::None), value_type(VMValueType::Array),  value_typedef(NULL), cr_value(NULL), n_value(NULL), m_value(a)  { }
    VMValue(const VMString& s)                                : value_flags(VMValueFlags::None), value_type(VMValueType::String), value_typedef(NULL), cr_value(NULL), n_value(NULL), s_value(s)  { }
    VMValue(bool b)                                           : value_flags(VMValueFlags::None), value_type(VMValueType::Bool),   value_typedef(NULL), cr_value(NULL), n_value(NULL), ui_value(b) { }
    VMValue(int64_t i)                                        : value_flags(VMValueFlags::None), value_type(VMValueType::s64),    value_typedef(NULL), cr_value(NULL), n_value(NULL), si_value(i) { guess_size(true);  }
    VMValue(uint64_t i)                                       : value_flags(VMValueFlags::None), value_type(VMValueType::u64),    value_typedef(NULL), cr_value(NULL), n_value(NULL), ui_value(i) { guess_size(false); }
    VMValue(float f)                                          : value_flags(VMValueFlags::None), value_type(VMValueType::Float),  value_typedef(NULL), cr_value(NULL), n_value(NULL), d_value(f)  { }
    VMValue(double d)                                         : value_flags(VMValueFlags::None), value_type(VMValueType::Double), value_typedef(NULL), cr_value(NULL), n_value(NULL), d_value(d)  { }

    static VMValue build(uint64_t bits, bool issigned, bool isfp);
    static VMValue build_array(uint64_t size = 0);
    static VMValue build_string(uint64_t size = 0);
    static VMValue build_string(const std::string& s);
    static VMValue build_string_reference(VMValue *from, const VMValue& index);
    static VMValue build_default_integer();

    void guess_size(bool issigned);

    bool is_const() const;
    bool is_local() const;
    bool is_reference() const;

    bool is_null() const;
    bool is_string() const;
    bool is_array() const;
    bool is_node() const;
    bool is_integer() const;
    bool is_floating_point() const;
    bool is_scalar() const;

    bool is_negative() const;
    bool is_signed() const;

    std::shared_ptr<VMValue> is_member(const std::string& member) const;

    std::string type_name() const;

    const char* to_string() const;

    void assign(const VMValue& rhs);
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

    std::shared_ptr<VMValue> operator[](const VMValue& index);
    std::shared_ptr<VMValue> operator[](const std::string& member);

    size_t                                  value_flags;
    size_t                                  value_type;
    Node*                                   value_typedef;
    std::string                             value_id;
    char*                                   cr_value;  // VMString's reference
    Node*                                   n_value;
    std::vector< std::shared_ptr<VMValue> > m_value;
    VMString                                s_value;

    union
    {
        int64_t  si_value;
        uint64_t ui_value;
        double   d_value;
    };
};

struct VMValueHasher
{
    std::size_t operator()(const VMValue& key) const
    {
        if(key.is_integer())
            return std::hash<uint64_t>()(key.ui_value);
        else if(key.is_floating_point())
            return std::hash<double>()(key.d_value);
        else if(key.is_string())
            return std::hash<std::string>()(key.s_value.data());

        throw std::runtime_error("Cannot hash '" + key.type_name() + "'");
        return std::size_t();
    }
};

typedef std::shared_ptr<VMValue> VMValuePtr;
typedef std::vector<VMValuePtr> VMValueMembers;

#endif // VMVALUE_H
