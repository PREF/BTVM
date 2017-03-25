#ifndef BTVM_FUNCTIONS_H
#define BTVM_FUNCTIONS_H

#include <string>
#include <vector>
#include "vmvalue.h"
#include "ast.h"

#define VMUnused(x) (void)x
#define PLATFORM_BITS 8

enum VMState { NoState = 0,
               Error,
               Break, Continue, Return };

namespace VMFunctions {

using namespace std;

typedef vector<VMValuePtr> ValueList;

template<typename T> string number_to_string(T num, int base, size_t fill = 0)
{
    static const char* digits = "0123456789abcdef";
    string value;

    do
    {
        value.insert(0, &digits[num % base], 1);
        num /= base;
    }
    while(num != 0);

    if((base == 16) && (value.length() < fill))
        value = string(fill - value.length(), '0').append(value);

    if(base == 16)
    {
        std::transform(value.begin(), value.end(), value.begin(), ::toupper);
        value += "h";
    }
    else if(base == 8)
        value += "o";
    else if(base == 2)
        value += "b";

    return value;
}

inline int state_check(int* state)
{
    if(*state == VMState::NoState)
        return *state;

    int s = *state;

    if((*state != VMState::Return) && (*state != VMState::Error))
        *state = VMState::NoState;

    return s;
}

inline int64_t string_to_number(const string& s, int base) { return strtoul(s.c_str(), NULL, base); }
inline double string_to_number(const string& s) { return atof(s.c_str()); }
string format_string(const VMValuePtr &format, const ValueList& args);
string node_typeid(Node* node);
VMValueType::VMType integer_literal_type(int64_t value);
VMValueType::VMType scalar_type(uint64_t bits, bool issigned, bool isfp);
VMValueType::VMType value_type(Node *node);
bool is_type_compatible(const VMValuePtr& vmvalue1, const VMValuePtr& vmvalue2);
bool type_cast(const VMValuePtr& vmvalue, Node *node);
void change_sign(const VMValuePtr& vmvalue);
size_t type_width(VMValueType::VMType valuetype);

}

#endif // BTVM_FUNCTIONS_H
