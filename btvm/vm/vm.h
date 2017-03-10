#ifndef VM_H
#define VM_H

#include <unordered_map>
#include <functional>
#include <string>
#include <deque>
#include <list>
#include "ast/ast.h"

#define VMUnused(x) (void)x
#define ScopeContext(x) VM::VMScopeContext __scope__(x)

class VM
{
    private:
        struct VMScopeContext
        {
            public:
                VMScopeContext(VM* vm): _vm(vm) { this->_vm->scopestack.push_back(VMVariables()); }
                ~VMScopeContext() { this->_vm->scopestack.pop_back(); }

            private:
                VM* _vm;
        };

    protected:
        typedef std::function<VMValuePtr(VM*, NCall*)> VMFunction;
        typedef std::unordered_map<std::string, VMValuePtr> VMVariables;
        typedef std::unordered_map<std::string, Node*> VMDeclarations;
        typedef std::unordered_map<std::string, VMFunction> VMFunctionsMap;
        typedef std::deque<VMVariables> VMScopeStack;
        typedef std::deque<VMValuePtr> VMDeclarationStack;

    public:
        VM();
        ~VM();
        VMValuePtr execute(const std::string& file);
        virtual void evaluate(const std::string& code);
        void dump(const std::string& file, const std::string& astfile);
        void loadAST(NBlock* ast);

    public: // Error management
        virtual VMValuePtr error(const std::string& msg);
        void syntaxError(const std::string& token, unsigned int line);

    private:
        bool checkArguments(NFunction* nfunc, NCall* ncall);
        bool checkArraySize(const VMValuePtr& arraysize);
        Node* getSize(NVariable* nvar);

    protected:
        virtual void onAllocating(const VMValuePtr& vmvalue);
        void declare(NIdentifier *nid, Node* node);
        uint64_t sizeOf(Node* node);
        uint64_t sizeOf(const NodeList& nodes);
        uint64_t sizeOf(const VMValuePtr& vmvalue);
        uint64_t sizeOf(const VMValueMembers& vmvmembers);
        Node *declaration(Node* ndecl);
        Node *declaration(NIdentifier* nid);
        VMValuePtr variable(NIdentifier* id);

    private: // Allocations
        void initVariable(const VMValuePtr &vmtype, const VMValuePtr& vmvalue, const VMValuePtr& defaultvalue = NULL);
        void bindVariables(NVariable* nvar);
        void bindVariable(NVariable* nvar);
        bool allocArguments(NFunction* nfunc, NCall* ncall);
        VMValuePtr allocVariable(NVariable* nvar);

    private: // Call and scope
        void call(NCall* ncall);
        void callVM(NCall* ncall);

    private: // Declarations
        Node* isDeclared(NIdentifier *nid) const;
        bool isVMFunction(NIdentifier* id) const;

    private:
        std::string readFile(const std::string& file) const;
        void writeFile(const std::string& file, const std::string& data) const;

    protected:
        VMDeclarations declarations;
        VMFunctionsMap functions;
        VMValuePtr result;
        NBlock* ast;

    protected: // AST data
        VMDeclarationStack declarationstack;
        VMScopeStack scopestack;
        VMVariables globals;
        int state;

    friend class NCompareOperator;
    friend class NBinaryOperator;
    friend class NUnaryOperator;
    friend class NDotOperator;
    friend class NSizeOf;
    friend class NVMState;
    friend class NFunction;
    friend class NIdentifier;
    friend class NVariable;
    friend class NArgument;
    friend class NType;
    friend class NBasicType;
    friend class NCompoundType;
    friend class NTypedef;
    friend class NReturn;
    friend class NBlock;
    friend class NScope;
    friend class NEnum;
    friend class NCall;
    friend class NSwitch;
    friend class NDoWhile;
    friend class NWhile;
    friend class NFor;
    friend class Node;
};

#endif // VM_H
