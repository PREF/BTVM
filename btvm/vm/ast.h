#ifndef AST_H
#define AST_H

#include <algorithm>
#include <unordered_map>
#include <cstdint>
#include <string>
#include <vector>
#include "vmvalue.h"

class VM;
struct Node;

typedef std::vector<Node*> NodeList;
struct delete_node { template<typename T> void operator()(T t) { delete t; } };

#define AST_NODE(x)         virtual std::string __type__() const { return #x; }
#define delete_nodelist(n)  std::for_each(n.begin(), n.end(), delete_node())
#define delete_if(n)        if(n) delete n

#define anonymous_type_prefix        "__anonymous_decl__"
#define anonymous_identifier         (new NIdentifier(anonymous_type_prefix + std::to_string(global_id++) + "__"))
#define is_anonymous_identifier(sid) (sid.find(anonymous_type_prefix) == 0)

#define node_s_typename(n)  #n
#define node_typename(n)    ((n)->__type__())
#define node_is(n, t)       (n && ((n)->__type__() == #t))
#define node_inherits(n, t) (n && dynamic_cast<t*>(n))
#define node_is_compound(n) node_inherits(n, NCompoundType)

struct Node
{
    Node() { }
    virtual ~Node() { }
    virtual std::string __type__() const = 0;

    protected:
        static unsigned long long global_id;
};

// ---------------------------------------------------------
//                      Basic Nodes
// ---------------------------------------------------------

struct NLiteral: public Node
{
    AST_NODE(NLiteral)

    NLiteral(): Node() { }
};

struct NBoolean: public NLiteral
{
    AST_NODE(NBoolean)

    NBoolean(bool value): NLiteral(), value(value) { }

    bool value;
};

struct NInteger: public NLiteral
{
    AST_NODE(NInteger)

    NInteger(int64_t value): NLiteral(), value(value) { }

    int64_t value;
};

struct NReal: public NLiteral
{
    AST_NODE(NReal)

    NReal(double value): NLiteral(), value(value) { }

    double value;
};

struct NString: public NLiteral
{
    AST_NODE(NString)

    NString(const std::string& value): NLiteral(), value(value) { }

    std::string value;
};

struct NIdentifier: public Node
{
    AST_NODE(NIdentifier)

    NIdentifier(const std::string& value): Node(), value(value) { }

    std::string value;
};

struct NEnumValue: public Node
{
    AST_NODE(NEnumValue)

    NEnumValue(NIdentifier* id): Node(), name(id), value(NULL) { }
    NEnumValue(NIdentifier* id, Node* value): Node(), name(id), value(value) { }
    ~NEnumValue() { delete name; delete_if(value); }

    NIdentifier* name;
    Node* value;
};

struct NCustomVariable: public Node
{
    AST_NODE(NCustomVariable)

    NCustomVariable(const std::string& action, Node* value): Node(), action(action), value(value) { }

    const std::string& action;
    Node* value;
};

struct NType: public Node
{
    AST_NODE(NType)

    NType(const std::string& name): Node(), name(new NIdentifier(name)), size(NULL), is_basic(false), is_compound(false) { }
    NType(NIdentifier* name, const NodeList& customvars): Node(), name(name), size(NULL), custom_vars(customvars), is_basic(false), is_compound(false) { }
    NType(NIdentifier* name): Node(), name(name), size(NULL), is_basic(false), is_compound(false) { }
    ~NType() { delete name; delete_if(size); delete_nodelist(custom_vars); }

    NIdentifier* name;
    Node* size;
    NodeList custom_vars;
    bool is_basic;
    bool is_compound;
};

struct NBasicType: public NType
{
    AST_NODE(NBasicType)

    NBasicType(const std::string& name, uint64_t bits): NType(name), bits(bits) { is_basic = true; }

    uint64_t bits;
};

struct NCompoundType: public NType
{
    AST_NODE(NCompoundType)

    NCompoundType(const NodeList& fields): NType(anonymous_identifier), members(fields) { is_basic = false; is_compound = true; }
    NCompoundType(NIdentifier* id, const NodeList& fields): NType(id), members(fields) { is_basic = false; is_compound = true; }
    NCompoundType(const NodeList& fields, const NodeList& customvars): NType(anonymous_identifier, customvars), members(fields) { is_basic = false; is_compound = true; }
    NCompoundType(NIdentifier* id, const NodeList& fields, const NodeList& customvars): NType(id, customvars), members(fields) { is_basic = false; is_compound = true; }

    NodeList members;
};

struct NBooleanType: public NBasicType
{
    AST_NODE(NBooleanType)

    NBooleanType(const std::string& name): NBasicType(name, 8) { }
};

struct NScalarType: public NBasicType
{
    AST_NODE(NScalarType)

    NScalarType(const std::string& name, uint64_t bits): NBasicType(name, bits), is_signed(true), is_fp(false) { }

    bool is_signed;
    bool is_fp;
};


struct NStringType: public NBasicType
{
    AST_NODE(NStringType)

    NStringType(const std::string& name): NBasicType(name, 8) { }
};

struct NDosDate: public NScalarType
{
    AST_NODE(NDosDate)

    NDosDate(const std::string& name): NScalarType(name, 16) { is_signed = false; }
};

struct NDosTime: public NScalarType
{
    AST_NODE(NDosTime)

    NDosTime(const std::string& name): NScalarType(name, 16) { is_signed = false; }
};

struct NTime: public NScalarType
{
    AST_NODE(NTime)

    NTime(const std::string& name): NScalarType(name, 32) { is_signed = false; }
};

struct NFileTime: public NScalarType
{
    AST_NODE(NFileTime)

    NFileTime(const std::string& name): NScalarType(name, 64) { is_signed = false; }
};

struct NOleTime: public NScalarType
{
    AST_NODE(NOleTime)

    NOleTime(const std::string& name): NScalarType(name, 64) { is_signed = false; }
};

struct NEnum: public NCompoundType
{
    AST_NODE(NEnum)

    NEnum(const NodeList& members, NType* type): NCompoundType(members), type(type) { this->type = (type ? type : new NScalarType("int", 32)); }
    NEnum(NIdentifier* id, const NodeList& members, NType* type): NCompoundType(id, members), type(type) { this->type = (type ? type : new NScalarType("int", 32)); }
    NEnum(const NodeList& members, const NodeList& customvars, NType* type): NCompoundType(members, customvars), type(type) { this->type = (type ? type : new NScalarType("int", 32)); }
    NEnum(NIdentifier* id, const NodeList& members, const NodeList& customvars, NType* type): NCompoundType(id, members, customvars), type(type) { this->type = (type ? type : new NScalarType("int", 32)); }
    ~NEnum() { delete_if(type); }

    NType* type;
};

struct NBlock: public Node
{
    AST_NODE(NBlock)

    NBlock(): Node() { }
    NBlock(const NodeList& statements): Node(), statements(statements) { }
    ~NBlock() { delete_nodelist(statements); }

    NodeList statements;
};

struct NVariable: public Node
{
    AST_NODE(NVariable)

    NVariable(Node* bits): Node(), type(NULL), name(anonymous_identifier), value(NULL), size(NULL), bits(bits), is_const(false), is_local(false) { }
    NVariable(NIdentifier* name, Node* size): Node(), type(NULL), name(name), value(NULL), size(size), bits(NULL), is_const(false), is_local(false) { }
    NVariable(NIdentifier* name, Node* size, Node* bits): Node(), type(NULL), name(name), value(NULL), size(size), bits(bits), is_const(false), is_local(false) { }
    virtual ~NVariable() { delete type; delete name; delete_if(value); delete size; }

    Node* type;
    NIdentifier* name;
    NodeList names;
    NodeList custom_vars;
    Node* value;
    Node* size;
    Node* bits;
    bool is_const;
    bool is_local;
};

struct NArgument: public NVariable
{
    AST_NODE(NArgument)

    NArgument(Node* type, NIdentifier* name, Node* size): NVariable(name, size), by_reference(false) { this->type = type; this->is_local = true; }

    bool by_reference;
};

// ---------------------------------------------------------
//                     Statement Nodes
// ---------------------------------------------------------

struct NReturn: public Node
{
    AST_NODE(NReturn)

    NReturn(NBlock* block): Node(), block(block) { }
    ~NReturn() { delete block; }

    NBlock* block;
};

struct NBinaryOperator: public Node
{
    AST_NODE(NBinaryOperator)

    NBinaryOperator(Node* left, const std::string& op, Node* right): Node(), left(left), op(op), right(right) { }
    ~NBinaryOperator() { delete left; delete right; }

    Node* left;
    std::string op;
    Node* right;
};

struct NUnaryOperator: public Node
{
    AST_NODE(NUnaryOperator)

    NUnaryOperator(const std::string& op, Node* expression, bool prefix): Node(), op(op), expression(expression), is_prefix(prefix) { }
    ~NUnaryOperator() { delete expression; }

    std::string op;
    Node* expression;
    bool is_prefix;
};

struct NDotOperator: public NBinaryOperator
{
    AST_NODE(NDotOperator)

    NDotOperator(Node* left, NIdentifier* right): NBinaryOperator(left, ".", right) { }
};

struct NIndexOperator: public Node
{
    AST_NODE(NIndexOperator)

    NIndexOperator(Node* expression, Node* index): Node(), expression(expression), index(index) { }
    ~NIndexOperator() { delete index; }

    Node* expression;
    Node* index;
};

struct NCompareOperator: public Node
{
    AST_NODE(NCompareOperator)

    NCompareOperator(Node* lhs, Node* rhs, const std::string& cmp): Node(), left(lhs), right(rhs), cmp(cmp) { }
    ~NCompareOperator() { delete left; delete right; }

    Node* left;
    Node* right;
    std::string cmp;
};

struct NConditional: public Node
{
    AST_NODE(NConditional)

    NConditional(Node* condition, Node* trueblock): Node(), condition(condition), true_block(trueblock), false_block(NULL) { }
    NConditional(Node* condition, Node* trueblock, Node* falseblock): Node(), condition(condition), true_block(trueblock), false_block(falseblock) { }
    ~NConditional() { delete condition; delete true_block; delete_if(false_block); }

    Node* condition;
    Node* true_block;
    Node* false_block;
};

struct NWhile: public NConditional
{
    AST_NODE(NWhile)

    NWhile(Node* condition, Node* trueblock): NConditional(condition, trueblock) { }
};

struct NDoWhile: public NWhile
{
    AST_NODE(NDoWhile)

    NDoWhile(Node* body, Node* condition): NWhile(condition, body) { }
};

struct NFor: public NConditional
{
    AST_NODE(NFor)

    NFor(Node* counter, Node* condition, Node* update, Node* trueblock): NConditional(condition, trueblock), counter(counter), update(update) { }
    ~NFor() { delete counter; delete update; }

    Node* counter;
    Node* update;
};

struct NSizeOf: public Node
{
    AST_NODE(NSizeOf)

    NSizeOf(Node* expression): Node(), expression(expression) { }
    ~NSizeOf() { delete expression; }

    Node* expression;
};

struct NVMState: public Node
{
    AST_NODE(NVMState)

    NVMState(int state): Node(), state(state) { }

    int state;
};

struct NCase: public Node
{
    AST_NODE(NCase)

    NCase(Node* body): Node(), value(NULL), body(body) { }
    NCase(Node* value, Node* body): Node(), value(value), body(body) { }
    ~NCase() { delete_if(value); delete body; }

    Node* value;
    Node* body;
};

struct NSwitch: public Node
{
    AST_NODE(NSwitch)

    NSwitch(Node* expression, const NodeList& cases): Node(), cases(cases), expression(expression), defaultcase(NULL) { }
    ~NSwitch() { delete expression; delete_if(defaultcase); /* delete_nodelist(cases); */ }

    NodeList cases;
    Node* expression;
    Node* defaultcase;
};

struct NStruct: public NCompoundType
{
    AST_NODE(NStruct)

    NStruct(const NodeList& members): NCompoundType(members) { }
    NStruct(NIdentifier* id, const NodeList& members): NCompoundType(id, members) { }
    NStruct(const NodeList& members, const NodeList& customvars): NCompoundType(members, customvars) { }
    NStruct(NIdentifier* id, const NodeList& members, const NodeList& customvars): NCompoundType(id, members, customvars) { }
};

struct NUnion: public NStruct
{
    AST_NODE(NUnion)

    NUnion(const NodeList& fields): NStruct(fields) { }
    NUnion(NIdentifier* id, const NodeList& fields): NStruct(id, fields) { }
    NUnion(const NodeList& fields, const NodeList& customvars): NStruct(fields,  customvars) { }
    NUnion(NIdentifier* id, const NodeList& fields, const NodeList& customvars): NStruct(id, fields, customvars) { }
};

struct NTypedef: public NType
{
    AST_NODE(NTypedef)

    NTypedef(Node* type, NIdentifier* name, const NodeList& customvars): NType(name, customvars), type(type) { is_basic = false; }
    virtual ~NTypedef() { delete type; }

    Node* type;
};

// ---------------------------------------------------------
//                     Function Nodes
// ---------------------------------------------------------

struct NFunction: public Node
{
    AST_NODE(NFunction)

    NFunction(NType* type, NIdentifier* name, NBlock* body): Node(), type(type), name(name), body(body) { }
    NFunction(NType* type, NIdentifier* name, const NodeList& arguments, NBlock* body): Node(), type(type), name(name), arguments(arguments), body(body) { }
    ~NFunction() { delete type; delete name; delete_nodelist(arguments); delete body; }

    NType* type;
    NIdentifier* name;
    NodeList arguments;
    NBlock* body;
};

// ---------------------------------------------------------
//                     Expression Nodes
// ---------------------------------------------------------

struct NCall: public Node
{
    AST_NODE(NCall)

    NCall(NIdentifier* name): name(name) { }
    NCall(NIdentifier* name, const NodeList& arguments): name(name), arguments(arguments) { }
    ~NCall() { delete name; delete_nodelist(arguments); }

    NIdentifier* name;
    NodeList arguments;
};

struct NCast: public Node
{
    AST_NODE(NCast)

    NCast(Node* cast, Node* expression): Node(), cast(cast), expression(expression) { }

    Node* cast;
    Node* expression;
};

std::string dump_ast(Node* n);

#endif // AST_H
