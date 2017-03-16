#ifndef VM_H
#define VM_H

#include <unordered_map>
#include <functional>
#include <string>
#include <deque>
#include <list>
#include "ast.h"

#define VMUnused(x) (void)x
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
        void declare(Node* node);
        void declareVariables(NVariable* nvar);
        void declareVariable(NVariable* nvar);
        VMValuePtr call(NCall* ncall);
        VMValuePtr callVM(NCall* ncall);
        VMValuePtr allocType(Node *node, Node* size = NULL);
        VMValuePtr allocVariable(NVariable* nvar);
        VMValuePtr variable(NIdentifier* id);
        Node* arraySize(NVariable* nvar);
        Node* declaration(Node* node);
        Node* isDeclared(NIdentifier *nid) const;
        bool isSizeValid(const VMValuePtr& vmvalue);
        bool isVMFunction(NIdentifier* id) const;
        bool pushScope(NFunction* nfunc, NCall* ncall);
        VMCaseMap buildCaseMap(NSwitch* nswitch);
        std::string readFile(const std::string& file) const;
        void writeFile(const std::string& file, const std::string& data) const;
        void readValue(const VMValuePtr& vmvar);

    public: // Error management
        virtual VMValuePtr error(const std::string& msg);
        VMValuePtr argumentError(NCall* ncall, size_t expected);
        VMValuePtr typeError(Node* n, const std::string& expected);
        VMValuePtr typeError(const VMValuePtr& vmvalue, const std::string& expected);
        void syntaxError(const std::string& token, unsigned int line);

    protected:
        virtual void readValue(const VMValuePtr& vmvar, uint64_t size) = 0;
        virtual void processFormat(const VMValuePtr& vmvar) = 0;
        int64_t sizeOf(const VMValuePtr& vmvalue);
        int64_t sizeOf(NIdentifier* nid);
        int64_t sizeOf(NVariable* nvar);
        int64_t sizeOf(Node* node);

    protected:
        template<typename T> T symbol(NIdentifier* nid, std::function<T(const VMScope&, NIdentifier*)> cb) const;
        template<typename T> int64_t sizeOf(const std::vector<T> &v);

    private:
        VMSwitchMap _switchmap;
        VMDeclarationStack _declarationstack;
        VMScopeStack _scopestack;
        VMScope _globalscope;
        NBlock* _ast;

    protected:
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

template<typename T> int64_t VM::sizeOf(const std::vector<T> &v)
{
    int64_t size = 0;

    for(auto it = v.begin(); it != v.end(); it++)
        size += this->sizeOf(*it);

    return size;
}

#endif // VM_H
