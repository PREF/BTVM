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

string format_string(const VMValuePtr& format, const ValueList& args)
{
    string s;
    int argidx = 0;

    for(const char* p = format->to_string(); *p != '\0'; p++)
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

bool is_type_compatible(const VMValuePtr &vmvalue1, const VMValuePtr &vmvalue2)
{
    if(vmvalue1->is_scalar() && vmvalue2->is_scalar())
        return true;

    if(vmvalue1->is_node() && vmvalue2->is_node())
        return node_typename(vmvalue1->n_value) == node_typename(vmvalue2->n_value);

    return vmvalue1->value_type == vmvalue2->value_type;
}

bool type_cast(const VMValuePtr &vmtype, const VMValuePtr &vmvalue)
{
    if(vmtype->value_type == vmvalue->value_type)
        return true;

    if(vmtype->is_scalar() && vmvalue->is_scalar())
    {
        if(vmtype->is_integer() && vmvalue->is_floating_point())
        {
            if(vmtype->is_signed())
                vmvalue->si_value = static_cast<int64_t>(vmvalue->d_value);
            else
                vmvalue->ui_value = static_cast<uint64_t>(vmvalue->d_value);
        }
        else if(vmtype->is_floating_point() && vmvalue->is_integer())
        {
            if(vmvalue->is_signed())
                vmvalue->d_value = static_cast<double>(vmvalue->si_value);
            else
                vmvalue->d_value = static_cast<double>(vmvalue->ui_value);
        }

        vmvalue->value_type = vmtype->value_type;
        return true;
    }

    return false;
}
}
