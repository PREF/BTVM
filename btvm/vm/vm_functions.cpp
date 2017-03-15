#include "vm_functions.h"
#include <algorithm>
#include <cstdint>

namespace VMFunctions {

static VMValuePtr get_arg(const ValueList& args, size_t idx)
{
    if(idx >= args.size())
        return VMValuePtr();

    return args[idx];
}

static VMValueType::VMType literal_type(NLiteral* nliteral)
{
    if(node_is(nliteral, NString))
        return VMValueType::String;
    else if(node_is(nliteral, NBoolean))
        return VMValueType::Bool;
    else if(node_is(nliteral, NReal))
        return VMValueType::Double;
    else if(node_is(nliteral, NInteger))
        return VMFunctions::integer_literal_type(static_cast<NInteger*>(nliteral)->value);

    return VMValueType::Null;
}

string format_string(const VMValuePtr& format, const ValueList& args)
{
    string s;
    int argidx = 0;

    for(const char* p = format->s_value.data(); *p != '\0'; p++)
    {
        if(*p != '%') // Eat words
        {
            if(*p == '\\')
            {
                p++;

                if(*p == '"')
                    s += '\"';
                else if(*p == 't')
                    s += '\t';
                else if(*p == 'r')
                    s += '\r';
                else if(*p == 'n')
                    s += '\n';
                else
                    s += *p;

                continue;
            }

            s += *p;
            continue;
        }

        p++;

        string sw;
        double w = 0;

        while((!w && (*p == '-')) || ((*p >= '0') && (*p <= '9')) || (*p == '.')) // Eat width, if any
        {
            sw += *p;
            p++;
        }

        if(!sw.empty())
            w = atof(sw.c_str());

        switch(*p)
        {
            case 'd': // Signed integer
            case 'i': // Signed integer
                s += std::to_string(get_arg(args, argidx)->si_value);
                argidx++;
                continue;

            case 'u': // Unsigned integer
                s += std::to_string(get_arg(args, argidx)->ui_value);
                argidx++;
                continue;

            case 'x': // Hex integer
            case 'X': // Hex integer
            {
                string c = number_to_string(get_arg(args, argidx)->ui_value, 16);

                if(*p == 'X') // Uppercase...
                    std::transform(c.begin(), c.end(), c.begin(), ::toupper);

                s += c;
                argidx++;
                continue;
            }

            case 'o': // Octal integer
                s += number_to_string(get_arg(args, argidx)->ui_value, 8);
                argidx++;
                continue;

            case 'c': // Character
                s += static_cast<char>(get_arg(args, argidx)->ui_value);
                argidx++;
                continue;

            case 's': // String
                s += get_arg(args, argidx)->to_string();
                argidx++;
                continue;

            case 'f': // Float
            case 'e': // Float
            case 'g': // Float
                s += std::to_string(get_arg(args, argidx)->d_value);
                argidx++;
                continue;

            case 'l':
            {
                p++;

                if(*p == 'f')
                {
                    s += std::to_string(get_arg(args, argidx)->d_value);
                    argidx++;
                    continue;
                }

                argidx++;
                break;
            }

            case 'L':
            {
                p++;

                switch(*p)
                {
                    case 'd':
                        s += std::to_string(get_arg(args, argidx)->si_value);
                        argidx++;
                        continue;

                    case 'u':
                        s += std::to_string(get_arg(args, argidx)->ui_value);
                        argidx++;
                        continue;

                    case 'x':
                    case 'X':
                    {
                        string c = number_to_string(get_arg(args, argidx)->ui_value, 16);

                        if(*p == 'X') // Uppercase...
                            std::transform(c.begin(), c.end(), c.begin(), ::toupper);

                        s += c;
                        argidx++;
                        continue;
                    }
                }

            }

            default:
                break;
        }

        s += *p;
    }

    return s;
}

VMValueType::VMType integer_literal_type(uint64_t value)
{
    if(value <= 0xFFFFFFFF)
        return VMValueType::u32;

    return VMValueType::u64;
}

VMValueType::VMType integer_literal_type(int64_t value)
{
    if(value <= 0xFFFFFFFF)
        return VMValueType::s32;

    return VMValueType::s64;
}

VMValueType::VMType scalar_type(uint64_t bits, bool issigned, bool isfp)
{
    if(isfp)
        return (bits < 64) ? VMValueType::Float : VMValueType::Double;

    if(bits <= 8)
        return issigned ? VMValueType::s8 : VMValueType::u8;

    if(bits <= 16)
        return issigned ? VMValueType::s16 : VMValueType::u16;

    if(bits <= 32)
        return issigned ? VMValueType::s32 : VMValueType::u32;

    return issigned ? VMValueType::s64 : VMValueType::u64;
}


VMValueType::VMType value_type(Node* node)
{
    VMValueType::VMType valuetype = VMValueType::Null;

    if(node_is(node, NScalarType))
    {
        NScalarType* nscalartype = static_cast<NScalarType*>(node);
        valuetype = VMFunctions::scalar_type(nscalartype->bits, nscalartype->is_signed, nscalartype->is_fp);
    }
    else if(node_inherits(node, NLiteral))
        valuetype = VMFunctions::literal_type(static_cast<NLiteral*>(node));

    if(valuetype == VMValueType::Null)
        throw std::runtime_error("Cannot get value-type of '" + node_typename(node) + "'");

    return valuetype;
}

bool is_type_compatible(const VMValuePtr &vmvalue1, const VMValuePtr &vmvalue2)
{
    if(vmvalue1->is_scalar() && vmvalue2->is_scalar())
        return true;

    if(vmvalue1->is_compound() && vmvalue2->is_compound())
        return node_typename(vmvalue1->value_typedef) == node_typename(vmvalue2->value_typedef);

    return vmvalue1->value_type == vmvalue2->value_type;
}

bool type_cast(const VMValuePtr &vmvalue, Node* node)
{
    if(vmvalue->is_compound())
        return node_typename(vmvalue->value_typedef) == node_typename(node);

    if(node_is(node, NScalarType) && vmvalue->is_scalar())
    {
        NScalarType* nscalartype = static_cast<NScalarType*>(node);

        if(!nscalartype->is_fp && vmvalue->is_floating_point())
        {
            if(nscalartype->is_signed)
                vmvalue->si_value = static_cast<int64_t>(vmvalue->d_value);
            else
                vmvalue->ui_value = static_cast<uint64_t>(vmvalue->d_value);
        }
        else if(nscalartype->is_fp && vmvalue->is_integer())
        {
            if(vmvalue->is_signed())
                vmvalue->d_value = static_cast<double>(vmvalue->si_value);
            else
                vmvalue->d_value = static_cast<double>(vmvalue->ui_value);
        }

        vmvalue->value_type = VMFunctions::value_type(node); // NOTE: Handle sign
        return true;
    }

    return false;
}

}
