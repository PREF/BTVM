#include "ast.h"
#include "../vm_functions.h"

using namespace std;

unsigned long long Node::global_id = 0;

#define dump_object_name __xml_dump__
#define create_dump_object(n) XMLNode dump_object_name(node_typename(n));
#define return_dump_object return dump_object_name

#define get_attribute(n, t, a) (static_cast<t*>(n)->a)
#define is_attribute_valid get_attribute

#define set_simple_attribute(a)      dump_object_name.attribute = a
#define set_main_attribute(n, t, a)  dump_object_name.attribute = get_attribute(n, t, a)
#define dump_main_attribute(n, t, a) dump_object_name.attribute = dump_ast(get_attribute(n, t, a))
#define dump_node_attribute(n, t, a) dump_object_name.put(get_attribute(n, t, a), #a)

#define dump_attributes(n, t, a) std::for_each(get_attribute(n, t, a).begin(), \
                                               get_attribute(n, t, a).end(), \
                                               [&dump_object_name](Node* __n__) { dump_ast(__n__); })

#define xml_open_tag(x)         ("<" + x + ">\n")
#define xml_open_tag_attr(x, a) ("<" + x + " id=\"" + a + "\">\n")
#define xml_close_tag(x)        ("</" + x + ">\n")

struct XMLNode
{
    XMLNode(const string& name, const string& attribute = string()): name(name), attribute(attribute) { }
    void put(const string& s) { content += s; }

    void put(Node* node, const string& attribute = string())
    {
        XMLNode n(node->__type__(), attribute);
        n.put(dump_ast(node));

        this->put(n);
    }

    void put(const NodeList& nodes, const string& attribute = string())
    {
        XMLNode n("NodeList", attribute);

        for(auto it = nodes.begin(); it != nodes.end(); it++)
            n.put(dump_ast(*it));

        this->put(n);
    }

    operator string() const { return (attribute.empty() ? xml_open_tag(name) : xml_open_tag_attr(name, attribute)) + content + xml_close_tag(name); }

    string name;
    string attribute;
    string content;
};

string dump_ast(Node *n)
{
    create_dump_object(n);

    if(node_is(n, NVMState))
    {
        if(get_attribute(n, NVMState, state) == VMState::Continue)
            set_simple_attribute("continue");
        else if(get_attribute(n, NVMState, state) == VMState::Break)
            set_simple_attribute("break");
        else if(get_attribute(n, NVMState, state) == VMState::Return)
            set_simple_attribute("return");
        else
            set_simple_attribute(std::to_string(get_attribute(n, NVMState, state)));
    }
    else if(node_is(n, NCustomVariable))
    {
        set_main_attribute(n, NCustomVariable, action);
        dump_node_attribute(n, NCustomVariable, value);
    }
    else if(node_inherits(n, NVariable))
    {
        dump_main_attribute(n, NVariable, type);
        dump_node_attribute(n, NVariable, name);

        if(is_attribute_valid(n, NVariable, value))
            dump_node_attribute(n, NVariable, value);
    }
    else if(node_is(n, NFunction))
    {
        dump_main_attribute(n, NFunction, name);
        dump_node_attribute(n, NFunction, arguments);
        dump_node_attribute(n, NFunction, body);
    }
    else if(node_is(n, NEnumValue))
    {
        dump_main_attribute(n, NEnumValue, name);

        if(is_attribute_valid(n, NEnumValue, value))
            dump_node_attribute(n, NEnumValue, value);
    }
    else if(node_is(n, NEnum))
    {
        dump_main_attribute(n, NEnum, name);
        dump_node_attribute(n, NEnum, members);
    }
    else if(node_is(n, NCall))
    {
        dump_main_attribute(n, NCall, name);
        dump_node_attribute(n, NCall, arguments);
    }
    else if(node_is(n, NCast))
    {
        dump_main_attribute(n, NCast, cast);
        dump_node_attribute(n, NCast, expression);
    }
    else if(node_is(n, NConditional))
    {
        dump_node_attribute(n, NConditional, condition);
        dump_node_attribute(n, NConditional, true_block);
    }
    else if(node_is(n, NCompareOperator))
    {
        set_main_attribute(n, NCompareOperator, cmp);
        dump_node_attribute(n, NCompareOperator, left);
        dump_node_attribute(n, NCompareOperator, right);
    }
    else if(node_is(n, NCase))
    {
        if(is_attribute_valid(n, NCase, value))
            dump_main_attribute(n, NCase, value);
        else
            set_simple_attribute("default");

        dump_node_attribute(n, NCase, body);
    }
    else if(node_is(n, NFor))
    {
        dump_node_attribute(n, NFor, counter);
        dump_node_attribute(n, NFor, condition);
        dump_node_attribute(n, NFor, update);
    }
    else if(node_is(n, NTypedef))
    {
        dump_main_attribute(n, NTypedef, name);
        dump_node_attribute(n, NTypedef, type);
        dump_node_attribute(n, NTypedef, custom_vars);
    }
    else if(node_is(n, NIndexOperator))
    {
        dump_node_attribute(n, NIndexOperator, index);
        dump_node_attribute(n, NIndexOperator, expression);
    }
    else if(node_is(n, NUnaryOperator))
    {
        set_main_attribute(n, NUnaryOperator, op);
        dump_node_attribute(n, NUnaryOperator, expression);
    }
    else if(node_inherits(n, NBinaryOperator))
    {
        set_main_attribute(n, NBinaryOperator, op);
        dump_node_attribute(n, NBinaryOperator, left);
        dump_node_attribute(n, NBinaryOperator, right);
    }
    else if(node_inherits(n, NWhile))
    {
        dump_node_attribute(n, NWhile, condition);
        dump_node_attribute(n, NWhile, true_block);
    }
    else if(node_inherits(n, NCompoundType))
    {
        dump_main_attribute(n, NCompoundType, name);
        dump_node_attribute(n, NCompoundType, members);
    }
    else if(node_is(n, NSizeOf))
        dump_node_attribute(n, NSizeOf, expression);
    else if(node_is(n, NReturn))
        dump_node_attribute(n, NReturn, block);
    else if(node_is(n, NBlock))
        dump_node_attribute(n, NBlock, statements);
    else if(node_is(n, NSwitch))
        dump_attributes(n, NSwitch, cases);
    else if(node_is(n, NIdentifier))
        return get_attribute(n, NIdentifier, value);
    else if(node_is(n, NString))
        return get_attribute(n, NString, value);
    else if(node_is(n, NBoolean))
        return get_attribute(n, NBoolean, value) ? "true" : "false";
    else if(node_is(n, NInteger))
        return std::to_string(get_attribute(n, NInteger, value));
    else if(node_is(n, NReal))
        return std::to_string(get_attribute(n, NReal, value));
    else if(node_inherits(n, NType))
        return dump_ast(static_cast<NType*>(n)->name);
    else
        throw std::runtime_error("Cannot dump '" + node_typename(n) + "'");

    return_dump_object;
}
