#ifndef VM_H
#define VM_H

#include <unordered_map>
#include <functional>
#include <string>
#include <deque>
#include <list>
#include "ast.h"
#include "vm_functions.h"

#define ScopeContext(x) VM::VMScopeContext __scope__(x)

class VM
{
    protected:
        typedef std::function<VMValuePtr(VM*, NCall*)> VMFunction;
        typedef std::unordered_map<std::string, VMValuePtr> VMVariables;
        typedef std::unordered_map<std::string, Node*> VMDeclarations;
        typedef std::unordered_map<std::string, VMFunction> VMFunctionsMap;

    private:
        struct VMScope {
            VMScope() { }
            VMScope(VMVariables v): variables(v) { }

            VMVariables variables;
            VMDeclarations declarations;
        };

        struct VMScopeContext {
            VMScopeContext(VM* vm): _vm(vm) { this->_vm->_scopestack.push_back(VMScope()); }
            ~VMScopeContext() { this->_vm->_scopestack.pop_back(); }

            private:
                VM* _vm;
        };

    private:
        typedef std::unordered_map<VMValue, uint64_t, VMValueHasher> VMCaseMap;
        typedef std::unordered_map<NSwitch*, VMCaseMap> VMSwitchMap;

    protected:
        typedef std::deque<VMScope> VMScopeStack;
        typedef std::deque<VMValuePtr> VMDeclarationStack;

    public:
        VM();
        ~VM();
        VMValuePtr execute(const std::string& file);
        virtual void evaluate(const std::string& code);
        void dump(const std::string& file, const std::string& astfile);
        VMValuePtr interpret(Node* node);
        void loadAST(NBlock* _ast);

    private:
        VMValuePtr interpret(const NodeList& nodelist);
        VMValuePtr interpret(NConditional* nconditional);
        VMValuePtr interpret(NDoWhile* ndowhile);
        VMValuePtr interpret(NWhile* nwhile);
        VMValuePtr interpret(NFor* nfor);
        VMValuePtr interpret(NSwitch* nswitch);
        VMValuePtr interpret(NCompareOperator* ncompare);
        VMValuePtr interpret(NUnaryOperator* nunary);
        VMValuePtr interpret(NBinaryOperator* nbinary);
        VMValuePtr interpret(NIndexOperator* nindex);
        VMValuePtr interpret(NDotOperator* ndot);
        VMValuePtr interpret(NReturn* nreturn);
        VMValuePtr interpret(NCast* ncast);
        VMValuePtr interpret(NSizeOf* nsizeof);
        VMValuePtr interpret(NEnum* nenum);
        void declare(Node* node);
        void declareVariables(NVariable* nvar);
        void declareVariable(NVariable* nvar);
        VMValuePtr call(NCall* ncall);
        VMValuePtr callVM(NCall* ncall);
        void allocType(const VMValuePtr& vmvar, Node *node, Node* nsize = NULL, const NodeList& nconstructor = NodeList());
        void allocVariable(const VMValuePtr &vmvar, NVariable* nvar);
        void allocEnum(NEnum* nenum, std::function<void(const VMValuePtr&)> cb);
        VMValuePtr variable(NIdentifier* id);
        Node* arraySize(NVariable* nvar);
        Node* declaration(Node* node);
        Node* isDeclared(NIdentifier *nid) const;
        bool isLocal(Node* node) const;
        bool isLocal(const VMValuePtr& vmvalue) const;
        bool isSizeValid(const VMValuePtr& vmvalue);
        bool isVMFunction(NIdentifier* id) const;
        bool pushScope(NIdentifier *nid, const NodeList &funcargs, const NodeList &callargs);
        VMCaseMap buildCaseMap(NSwitch* nswitch);
        int64_t getBits(const VMValuePtr& vmvalue);
        int64_t getBits(Node *n);
        std::string readFile(const std::string& file) const;
        void writeFile(const std::string& file, const std::string& data) const;
        void readValue(const VMValuePtr& vmvar, bool seek);

    public: // Error management
        virtual VMValuePtr error(const std::string& msg);
        VMValuePtr argumentError(NCall* ncall, size_t expected);
        VMValuePtr argumentError(NIdentifier* nid, const NodeList &given, const NodeList &expected);
        VMValuePtr typeError(Node* n, const std::string& expected);
        VMValuePtr typeError(const VMValuePtr& vmvalue, const std::string& expected);
        void syntaxError(const std::string& token, unsigned int line);

    protected:
        virtual void readValue(const VMValuePtr& vmvar, uint64_t size, bool seek) = 0;
        int64_t sizeOf(const VMValuePtr& vmvalue);
        int64_t sizeOf(NIdentifier* nid);
        int64_t sizeOf(NVariable* nvar);
        int64_t sizeOf(Node* node);

    private:
        template<typename T> T symbol(NIdentifier* nid, std::function<T(const VMScope&, NIdentifier*)> cb) const;
        template<typename T> int64_t unionSize(const std::vector<T> &v);
        template<typename T> int64_t compoundSize(const std::vector<T> &v);
        template<typename T> int64_t sizeOf(const std::vector<T> &v);

    private:
        VMSwitchMap _switchmap;
        VMDeclarationStack _declarationstack;
        VMScopeStack _scopestack;
        VMScope _globalscope;
        NBlock* _ast;

    protected:
        std::vector<VMValuePtr> allocations;
        VMFunctionsMap functions;
        int state;
};

template<typename T> T VM::symbol(NIdentifier *nid, std::function<T(const VMScope &, NIdentifier*)> cb) const
{
    T symbol;

    for(auto it = this->_scopestack.rbegin(); it != this->_scopestack.rend(); it++) // Loop through parent scopes
    {
        symbol = cb(*it, nid);

        if(!symbol)
            continue;

        return symbol;
    }

    return cb(this->_globalscope, nid); // Try globals
}

template<typename T> int64_t VM::unionSize(const std::vector<T> &v)
{
    int64_t maxsize = 0;

    for(auto it = v.begin(); it != v.end(); it++)
        maxsize = std::max(maxsize, this->sizeOf(*it));

    return maxsize;
}

template<typename T> int64_t VM::compoundSize(const std::vector<T> &v)
{
    uint64_t totbits = 0, bftotsize = 0, boundarybits = 0;

    for(auto it = v.begin(); it != v.end(); it++)
    {
        if(this->isLocal(*it))
            continue;

        uint64_t mbits = this->sizeOf(*it) * PLATFORM_BITS;
        boundarybits = std::max(mbits, boundarybits);
        int64_t bits = this->getBits(*it);

        if(bits > 0) // TODO: Handle bits == 0
        {
            totbits += bits;
            bftotsize += bits;
            continue;
        }

        totbits += mbits;

        if(bftotsize)
            totbits += (boundarybits - bftotsize);

        bftotsize = 0;
    }

    if(bftotsize)
        totbits += (boundarybits - bftotsize);

    return totbits / PLATFORM_BITS;
}

template<typename T> int64_t VM::sizeOf(const std::vector<T> &v)
{
    int64_t size = 0;

    for(auto it = v.begin(); it != v.end(); it++)
        size += this->sizeOf(*it);

    return size;
}

#endif // VM_H
