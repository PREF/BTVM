#ifndef AST_H
#define AST_H

#include <algorithm>
#include <unordered_map>
#include <cstdint>
#include <string>
#include <vector>
#include "../vmvalue.h"

class VM;
class Node;

typedef std::vector<Node*> NodeList;
struct delete_node { template<typename T> void operator()(T t) { delete t; } };

#define AST_NODE(x)         public: virtual std::string __type__() const { return #x; }
#define delete_nodelist(n)  std::for_each(n.begin(), n.end(), delete_node())
#define delete_if(n)        if(n) delete n

#define anonymous_type_prefix        "__anonymous_type_"
#define anonymous_identifier         (new NIdentifier(anonymous_type_prefix + std::to_string(__id__) + "__"))
#define is_anonymous_identifier(sid) (sid.find(anonymous_type_prefix) == 0)

#define node_s_typename(n) #n
#define node_typename(n)    ((n)->__type__())
#define node_is(n, t)       (n && ((n)->__type__() == #t))
#define node_inherits(n, t) (n && dynamic_cast<t*>(n))
#define node_is_compound(n) node_inherits(n, NCompoundType)

class Node
{
    public:
        Node() { __id__ = global_id++; }
        virtual ~Node() { }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM* vm);
        virtual std::string __type__() const = 0;

    public:
        unsigned long long __id__;

    private:
        static unsigned long long global_id;
};

// ---------------------------------------------------------
//                      Basic Nodes
// ---------------------------------------------------------

class NBoolean: public Node
{
    AST_NODE(NBoolean)

    public:
        NBoolean(bool value): Node(), value(value) { }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM* vm);

    public:
        bool value;

};

class NInteger: public Node
{
    AST_NODE(NInteger)

    public:
        NInteger(uint64_t value): Node(), value(value) { }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM* vm);

    public:
        uint64_t value;
};

class NReal: public Node
{
    AST_NODE(NReal)

    public:
        NReal(double value): Node(), value(value) { }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM* vm);

    public:
        double value;
};

class NLiteralString: public Node
{
    AST_NODE(NLiteralString)

    public:
        NLiteralString(const std::string& value): Node(), value(value) { }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM* vm);

    public:
        std::string value;
};

class NIdentifier: public Node
{
    AST_NODE(NIdentifier)

    public:
        NIdentifier(const std::string& value): Node(), value(value) { }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM* vm);

    public:
        std::string value;
};

class NEnumValue: public Node
{
    AST_NODE(NEnumValue)

    public:
        NEnumValue(NIdentifier* id): Node(), name(id), value(NULL) { }
        NEnumValue(NIdentifier* id, Node* value): Node(), name(id), value(value) { }
        ~NEnumValue() { delete name; delete_if(value); }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM* vm);

    public:
        NIdentifier* name;
        Node* value;
};

class NCustomVariable: public Node
{
    AST_NODE(NCustomVariable)

    public:
        NCustomVariable(const std::string& action, Node* value): Node(), action(action), value(value) { }
        virtual std::string dump() const;

    public:
        const std::string& action;
        Node* value;
};

class NType: public Node
{
    AST_NODE(NType)

    public:
        NType(const std::string& name): Node(), name(new NIdentifier(name)), size(NULL), is_basic(false), is_compound(false) { }
        NType(NIdentifier* name, const NodeList& customvars): Node(), name(name), size(NULL), custom_vars(customvars), is_basic(false), is_compound(false) { }
        NType(NIdentifier* name): Node(), name(name), size(NULL), is_basic(false), is_compound(false) { }
        ~NType() { delete name; delete_if(size); delete_nodelist(custom_vars); }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        NIdentifier* name;
        Node* size;
        NodeList custom_vars;
        bool is_basic;
        bool is_compound;
};

class NBasicType: public NType
{
    AST_NODE(NBasicType)

    public:
        NBasicType(const std::string& name, uint64_t bits): NType(name), bits(bits) { is_basic = true; }

    public:
        uint64_t bits;
};

class NCompoundType: public NType
{
    AST_NODE(NCompoundType)

    public:
        NCompoundType(const NodeList& fields): NType(anonymous_identifier), members(fields) { is_basic = false; is_compound = true; }
        NCompoundType(NIdentifier* id, const NodeList& fields): NType(id), members(fields) { is_basic = false; is_compound = true; }
        NCompoundType(const NodeList& fields, const NodeList& customvars): NType(anonymous_identifier, customvars), members(fields) { is_basic = false; is_compound = true; }
        NCompoundType(NIdentifier* id, const NodeList& fields, const NodeList& customvars): NType(id, customvars), members(fields) { is_basic = false; is_compound = true; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

   public:
        NodeList members;
};

class NBooleanType: public NBasicType
{
    AST_NODE(NBooleanType)

    public:
        NBooleanType(const std::string& name): NBasicType(name, 8) { }
        virtual VMValuePtr execute(VM *vm);
};

class NScalarType: public NBasicType
{
    AST_NODE(NScalarType)

    public:
        NScalarType(const std::string& name, uint64_t bits): NBasicType(name, bits), is_signed(true), is_fp(false) { }
        virtual VMValuePtr execute(VM *vm);

    public:
        bool is_signed;
        bool is_fp;
};


class NStringType: public NBasicType
{
    AST_NODE(NStringType)

    public:
        NStringType(const std::string& name): NBasicType(name, 8) { }
        virtual VMValuePtr execute(VM *vm);
};

class NDosDate: public NScalarType
{
    AST_NODE(NDosDate)

    public:
        NDosDate(const std::string& name): NScalarType(name, 16) { is_signed = false; }
};

class NDosTime: public NScalarType
{
    AST_NODE(NDosTime)

    public:
        NDosTime(const std::string& name): NScalarType(name, 16) { is_signed = false; }
};

class NTime: public NScalarType
{
    AST_NODE(NTime)

    public:
        NTime(const std::string& name): NScalarType(name, 32) { is_signed = false; }
};

class NFileTime: public NScalarType
{
    AST_NODE(NFileTime)

    public:
        NFileTime(const std::string& name): NScalarType(name, 64) { is_signed = false; }
};

class NOleTime: public NScalarType
{
    AST_NODE(NOleTime)

    public:
        NOleTime(const std::string& name): NScalarType(name, 64) { is_signed = false; }
};

class NEnum: public NCompoundType
{
    AST_NODE(NEnum)

    public:
        NEnum(const NodeList& members, NType* type): NCompoundType(members), type(type) { this->type = (type ? type : new NScalarType("int", 32)); }
        NEnum(NIdentifier* id, const NodeList& members, NType* type): NCompoundType(id, members), type(type) { this->type = (type ? type : new NScalarType("int", 32)); }
        NEnum(const NodeList& members, const NodeList& customvars, NType* type): NCompoundType(members, customvars), type(type) { this->type = (type ? type : new NScalarType("int", 32)); }
        NEnum(NIdentifier* id, const NodeList& members, const NodeList& customvars, NType* type): NCompoundType(id, members, customvars), type(type) { this->type = (type ? type : new NScalarType("int", 32)); }
        ~NEnum() { delete_if(type); }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        NType* type;
};

class NBlock: public Node
{
    AST_NODE(NBlock)

    public:
        NBlock(): Node() { }
        NBlock(const NodeList& statements): Node(), statements(statements) { }
        ~NBlock() { delete_nodelist(statements); }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM* vm);

    public:
        NodeList statements;
};

class NVariable: public Node
{
    AST_NODE(NVariable)

    public:
        NVariable(NIdentifier* name, Node* size): Node(), type(NULL), name(name), value(NULL), size(size), is_const(false), is_local(false) { }
        virtual ~NVariable() { delete type; delete name; delete_if(value); delete size; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        Node* type;
        NIdentifier* name;
        NodeList names;
        NodeList custom_vars;
        Node* value;
        Node* size;
        bool is_const;
        bool is_local;
};

class NArgument: public NVariable
{
    AST_NODE(NArgument)

    public:
        NArgument(Node* type, NIdentifier* name, Node* size): NVariable(name, size), by_reference(false) { this->type = type; this->is_local = true; }
        virtual VMValuePtr execute(VM *vm);

    public:
        bool by_reference;
};

// ---------------------------------------------------------
//                     Statement Nodes
// ---------------------------------------------------------

class NReturn: public Node
{
    AST_NODE(NReturn)

    public:
        NReturn(NBlock* block): Node(), block(block) { }
        ~NReturn() { delete block; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM* vm);

    public:
        NBlock* block;
};

class NBinaryOperator: public Node
{
    AST_NODE(NBinaryOperator)

    public:
        NBinaryOperator(Node* left, const std::string& op, Node* right): Node(), left(left), op(op), right(right) { }
        ~NBinaryOperator() { delete left; delete right; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        Node* left;
        std::string op;
        Node* right;
};

class NUnaryOperator: public Node
{
    AST_NODE(NUnaryOperator)

    public:
        NUnaryOperator(const std::string& op, Node* expression, bool prefix): Node(), op(op), expression(expression), is_prefix(prefix) { }
        ~NUnaryOperator() { delete expression; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        std::string op;
        Node* expression;
        bool is_prefix;
};

class NDotOperator: public NBinaryOperator
{
    AST_NODE(NDot)

    public:
        NDotOperator(Node* left, NIdentifier* right): NBinaryOperator(left, ".", right) { }
        virtual VMValuePtr execute(VM* vm);
};

class NIndexOperator: public Node
{
    AST_NODE(NIndexOperator)

    public:
        NIndexOperator(Node* expression, Node* index): Node(), expression(expression), index(index) { }
        ~NIndexOperator() { delete index; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
      Node* expression;
      Node* index;
};

class NCompareOperator: public Node
{
    AST_NODE(NCompareOperator)

    public:
        NCompareOperator(Node* lhs, Node* rhs, const std::string& cmp): Node(), left(lhs), right(rhs), cmp(cmp) { }
        ~NCompareOperator() { delete left; delete right; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        Node* left;
        Node* right;
        std::string cmp;
};

class NConditional: public Node
{
    AST_NODE(NConditionalOperator)

    public:
        NConditional(Node* condition, Node* ifbody): Node(), condition(condition), if_body(ifbody), body(NULL) { }
        NConditional(Node* condition, Node* ifbody, Node* body): Node(), condition(condition), if_body(ifbody), body(body) { }
        ~NConditional() { delete condition; delete if_body; delete_if(body); }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        Node* condition;
        Node* if_body;
        Node* body;
};

class NWhile: public Node
{
    AST_NODE(NWhile)

    public:
        NWhile(Node* condition, Node* body): Node(), condition(condition), body(body) { }
        ~NWhile() { delete condition; delete body; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        Node* condition;
        Node* body;
};

class NDoWhile: public NWhile
{
    AST_NODE(NDoWhile)

    public:
        NDoWhile(Node* body, Node* condition): NWhile(condition, body) { }
        virtual VMValuePtr execute(VM *vm);
};

class NFor: public Node
{
    AST_NODE(NFor)

    public:
        NFor(Node* counter, Node* condition, Node* update, Node* body): Node(), counter(counter), condition(condition), update(update), body(body) { }
        ~NFor() { delete counter; delete condition; delete update; delete body; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        Node* counter;
        Node* condition;
        Node* update;
        Node* body;
};

class NSizeOf: public Node
{
    AST_NODE(NSizeOf)

    public:
        NSizeOf(Node* expression): Node(), expression(expression) { }
        ~NSizeOf() { delete expression; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        Node* expression;
};

class NVMState: public Node
{
    AST_NODE(NVMState)

    public:
        NVMState(int state): Node(), state(state) { }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        int state;
};

class NCase: public Node
{
    AST_NODE(NCase)

    public:
        NCase(Node* body): Node(), value(NULL), body(body) { }
        NCase(Node* value, Node* body): Node(), value(value), body(body) { }
        ~NCase() { delete_if(value); delete body; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        Node* value;
        Node* body;
};

class NSwitch: public Node
{
    AST_NODE(NSwitch)

    public:
        NSwitch(Node* expression, const NodeList& cases): Node(), cases(cases), expression(expression), defaultcase(NULL) { }
        ~NSwitch() { delete expression; delete_if(defaultcase); /* delete_nodelist(cases); */ }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        std::unordered_map<VMValue, uint64_t, VMValueHasher> casemap;
        NodeList cases;
        Node* expression;
        Node* defaultcase;
};

class NStruct: public NCompoundType
{
    AST_NODE(NStruct)

    public:
        NStruct(const NodeList& members): NCompoundType(members) { }
        NStruct(NIdentifier* id, const NodeList& members): NCompoundType(id, members) { }
        NStruct(const NodeList& members, const NodeList& customvars): NCompoundType(members, customvars) { }
        NStruct(NIdentifier* id, const NodeList& members, const NodeList& customvars): NCompoundType(id, members, customvars) { }
};

class NUnion: public NStruct
{
    AST_NODE(NUnion)

    public:
        NUnion(const NodeList& fields): NStruct(fields) { }
        NUnion(NIdentifier* id, const NodeList& fields): NStruct(id, fields) { }
        NUnion(const NodeList& fields, const NodeList& customvars): NStruct(fields,  customvars) { }
        NUnion(NIdentifier* id, const NodeList& fields, const NodeList& customvars): NStruct(id, fields, customvars) { }
};

class NTypedef: public NType
{
    AST_NODE(NTypedef)

    public:
        NTypedef(Node* type, NIdentifier* name, const NodeList& customvars): NType(name, customvars), type(type) { is_basic = false; }
        virtual ~NTypedef() { delete type; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM *vm);

    public:
        Node* type;
};

// ---------------------------------------------------------
//                     Function Nodes
// ---------------------------------------------------------

class NFunction: public Node
{
    AST_NODE(NFunction)

    public:
        NFunction(NType* type, NIdentifier* name, NBlock* body): Node(), type(type), name(name), body(body) { }
        NFunction(NType* type, NIdentifier* name, const NodeList& arguments, NBlock* body): Node(), type(type), name(name), arguments(arguments), body(body) { }
        ~NFunction() { delete type; delete name; delete_nodelist(arguments); delete body; }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM* vm);

    public:
        NType* type;
        NIdentifier* name;
        NodeList arguments;
        NBlock* body;
};

// ---------------------------------------------------------
//                     Expression Nodes
// ---------------------------------------------------------

class NCall: public Node
{
    AST_NODE(NCall)

    public:
        NCall(NIdentifier* name): name(name) { }
        NCall(NIdentifier* name, const NodeList& arguments): name(name), arguments(arguments) { }
        ~NCall() { delete name; delete_nodelist(arguments); }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM* vm);

    public:
        NIdentifier* name;
        NodeList arguments;
};

class NCast: public Node
{
    AST_NODE(NCast)

    public:
        NCast(Node* cast, Node* expression): Node(), cast(cast), expression(expression) { }
        virtual std::string dump() const;
        virtual VMValuePtr execute(VM* vm);

    public:
        Node* cast;
        Node* expression;
};

#endif // AST_H
