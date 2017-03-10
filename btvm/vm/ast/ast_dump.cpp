#include "ast.h"
#include "../vm_functions.h"

using namespace std;

#define QVNode(x) ("\"" + x + "\"\n")
#define VNode(x) (x + "\n")
#define ONode(x) ("<" + x + ">\n")
#define ONode_a(x, a) ("<" + x + " id=\"" + a + "\">\n")
#define CNode(x) ("</" + x + ">\n")
#define XNode(x) XMLNode x(__type__())
#define XNode_a(x, a) XMLNode x(__type__(), a)
#define XNode_put(n, x) n.put(x)
#define XNode_put_a(n, x) n.put(x, #x)
#define XNodeC XMLNode(__type__())

struct XMLNode
{
    XMLNode(const string& name, const string& attribute = string()): name(name), attribute(attribute) { }
    void put(const string& s) { content += s; }

    void put(const Node* node, const string& attribute = string())
    {
        XMLNode n(node->__type__(), attribute);
        n.put(node->dump());

        this->put(n);
    }

    void put(const NodeList& nodes, const string& attribute = string())
    {
        XMLNode n("NodeList", attribute);

        for(auto it = nodes.begin(); it != nodes.end(); it++)
            n.put((*it)->dump());

        this->put(n);
    }

    operator string() const { return (attribute.empty() ? ONode(name) : ONode_a(name, attribute)) + content + CNode(name); }

    string name;
    string attribute;
    string content;
};

string Node::dump() const { return XNodeC; }
string NType::dump() const { return VNode(name->value); }
string NIdentifier::dump() const { return VNode(value); }
string NLiteralString::dump() const { return QVNode(value); }
string NBoolean::dump() const { return VNode(string(value ? "true" : "false")); }
string NInteger::dump() const { return VNode(std::to_string(value)); }
string NReal::dump() const { return VNode(std::to_string(value)); }

std::string NCustomVariable::dump() const
{
    XNode_a(n, action);
    XNode_put_a(n, value);
    return n;
}

string NBlock::dump() const
{
    XNode(n);
    XNode_put_a(n, statements);
    return n;
}

string NVariable::dump() const
{
    XNode_a(n, type->dump());
    XNode_put_a(n, name);

    if(value)
        XNode_put_a(n, value);

    return n;
}

string NFunction::dump() const
{
    XNode_a(n, name->value);
    XNode_put_a(n, name);
    XNode_put_a(n, arguments);
    XNode_put_a(n, body);
    return n;
}

string NEnumValue::dump() const
{
    XNode_a(n, name->value);
    XNode_put_a(n, name);

    if(value)
        XNode_put_a(n, value);

    return n;
}

string NEnum::dump() const
{
    XNode_a(n, name->value);
    XNode_put_a(n, name);
    XNode_put_a(n, members);
    return n;
}

string NSizeOf::dump() const
{
    XNode(n);
    XNode_put_a(n, expression);
    return n;
}

string NCall::dump() const
{
    XNode_a(n, name->value);
    XNode_put_a(n, name);
    XNode_put_a(n, arguments);
    return n;
}

string NCast::dump() const
{
    XNode_a(n, cast->dump());
    XNode_put_a(n, expression);
    return n;
}

string NReturn::dump() const
{
    XNode(n);
    XNode_put(n, block);
    return n;
}

string NBinaryOperator::dump() const
{
    XNode_a(n, op);
    XNode_put_a(n, left);
    XNode_put_a(n, right);
    return n;
}

string NUnaryOperator::dump() const
{
    XNode_a(n, op);
    XNode_put_a(n, expression);
    return n;
}

string NIndexOperator::dump() const
{
    XNode(n);
    XNode_put_a(n, expression);
    XNode_put_a(n, index);
    return n;
}

string NWhile::dump() const
{
    XNode(n);
    XNode_put_a(n, condition);
    XNode_put_a(n, body);
    return n;
}

string NFor::dump() const
{
    XNode(n);
    XNode_put_a(n, counter);
    XNode_put_a(n, condition);
    XNode_put_a(n, update);
    return n;
}

string NCompareOperator::dump() const
{
    XNode_a(n, cmp);
    XNode_put_a(n, right);
    XNode_put_a(n, right);
    return n;
}

string NConditional::dump() const
{
    XNode(n);
    XNode_put_a(n, condition);
    XNode_put_a(n, body);
    return n;
}

string NCase::dump() const
{
    XNode_a(n, value->dump());
    XNode_put_a(n, value);
    XNode_put_a(n, body);
    return n;
}

string NSwitch::dump() const
{
    XNode(n);
    std::for_each(cases.begin(), cases.end(), [this, &n](Node* node) { XNode_put_a(n, node); });
    return n;
}

string NCompoundType::dump() const
{
     XNode_a(n, name->value);
     XNode_put_a(n, members);
     return n;
}

string NTypedef::dump() const
{
    XNode_a(n, name->value);
    XNode_put_a(n, type);
    XNode_put_a(n, custom_vars);
    return n;
}

string NVMState::dump() const
{
    string s;

    if(state == VMState::Continue)
        s = "continue";
    else if(state == VMState::Break)
        s = "break";
    else
        s = std::to_string(state);

    XNode_a(n, s);
    return n;
}
