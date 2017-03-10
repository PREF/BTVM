#ifndef BTVM_H
#define BTVM_H

#include <unordered_map>
#include <string>
#include <stack>
#include "vm/vm.h"
#include "format/btentry.h"
#include "io/btvmio.h"

#define ColorInvalid 0xFFFFFFFF

class BTVM: public VM
{
    public:
        BTVM(BTVMIO* btvmio);
        ~BTVM();
        virtual void evaluate(const std::string& code);
        BTEntryList format();

    protected:
        virtual void print(const std::string& s);
        virtual void setBackColor(uint32_t rgb);
        virtual void setForeColor(uint32_t rgb);
        virtual void onAllocating(const VMValuePtr &vmvalue);

    private:
        BTEntryPtr buildEntry(const VMValuePtr& vmvalue, uint64_t &offset);
        void initFunctions();
        void initColors();

    private: // Interface Functions
        static VMValuePtr vmPrintf(VM *self, NCall* ncall);
        static VMValuePtr vmSetBackColor(VM *self, NCall* ncall);
        static VMValuePtr vmSetForeColor(VM *self, NCall* ncall);
        static VMValuePtr vmWarning(VM *self, NCall* ncall);

    private: // I/O Functions
        static VMValuePtr vmFEof(VM *self, NCall* ncall);
        static VMValuePtr vmFileSize(VM *self, NCall* ncall);
        static VMValuePtr vmFTell(VM *self, NCall* ncall);
        static VMValuePtr vmReadBytes(VM *self, NCall* ncall);
        static VMValuePtr vmReadUInt(VM *self, NCall* ncall);
        static VMValuePtr vmLittleEndian(VM *self, NCall* ncall);
        static VMValuePtr vmBigEndian(VM *self, NCall* ncall);

    private: // Math Functions
        static VMValuePtr vmCeil(VM* self, NCall* ncall);

    private: // Non-Standard Functions
        static VMValuePtr vmBtvmTest(VM *self, NCall* ncall);

    private:
        std::unordered_map<std::string, uint32_t> _colors;
        std::vector<VMValuePtr> _allocations;
        BTVMIO* _btvmio;
};

#endif // BTVM_H
