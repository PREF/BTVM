#include "btvm.h"
#include "bt_lexer.h"
#include "vm/vm_functions.h"
#include <iostream>
#include <cstdio>
#include <cmath>

#define ColorizeFail(s) "\x1b[31m" << s << "\x1b[0m"
#define ColorizeOk(s)   "\x1b[32m" << s << "\x1b[0m"

// Parser interface
void* BTParserAlloc(void* (*mallocproc)(size_t));
void BTParserFree(void* p, void (*freeproc)(void*));
void BTParser(void* yyp, int yymajor, BTLexer::Token* yyminor, BTVM* btvm);

BTVM::BTVM(BTVMIO *btvmio): VM(), _btvmio(btvmio)
{
    this->initFunctions();
    this->initColors();
}

BTVM::~BTVM()
{
    if(this->_btvmio)
    {
        delete this->_btvmio;
        this->_btvmio = NULL;
    }
}

void BTVM::evaluate(const string &code)
{
    VM::evaluate(code);
    this->_allocations.clear();

    BTLexer lexer(code.c_str());
    std::list<BTLexer::Token> tokens = lexer.lex();

    if(tokens.size() <= 0)
        return;

    void* parser = BTParserAlloc(&malloc);

    for(auto it = tokens.begin(); it != tokens.end(); it++)
    {
        if(this->state == VMState::Error)
            break;

        BTParser(parser, it->type, &(*it), this);
    }

    BTParser(parser, 0, NULL, this);
    BTParserFree(parser, &free);
}

BTEntryList BTVM::format()
{
    BTEntryList btfmt;

    if(this->state == VMState::NoState)
    {

        uint64_t offset = 0;

        for(auto it = this->_allocations.begin(); it != this->_allocations.end(); it++)
            btfmt.push_back(this->buildEntry(*it, offset));
    }
    else
        this->_allocations.clear();

    return btfmt;
}

void BTVM::print(const string &s)
{
    cout << s;
}

void BTVM::setBackColor(uint32_t rgb)
{
    cout << "Changing background to 0x" << hex << rgb << endl;
}

void BTVM::setForeColor(uint32_t rgb)
{
    cout << "Changing foreground to 0x" << hex << rgb << endl;
}

void BTVM::onAllocating(const VMValuePtr &vmvalue)
{
    if(vmvalue->is_const() || vmvalue->is_local())
        return;

    if(!vmvalue->is_node() && !vmvalue->is_array())
        this->_btvmio->read(vmvalue, this->sizeOf(vmvalue)); // Read value from file

    if(this->declarationstack.empty())
        this->_allocations.push_back(vmvalue);
}

BTEntryPtr BTVM::buildEntry(const VMValuePtr &vmvalue, uint64_t& offset)
{
    BTEntryPtr btentry = std::make_shared<BTEntry>(vmvalue, this->_btvmio->endianness());
    btentry->location = BTLocation(offset, this->sizeOf(vmvalue));

    if(vmvalue->is_array() || node_is_compound(vmvalue->n_value))
    {
        for(auto it = vmvalue->m_value.begin(); it != vmvalue->m_value.end(); it++)
            btentry->children.push_back(this->buildEntry(*it, offset));
    }
    else
        offset += this->sizeOf(vmvalue);

    return btentry;
}

void BTVM::initFunctions()
{
    // Interface Functions: https://www.sweetscape.com/010editor/manual/FuncInterface.htm
    this->functions["Printf"]        = &BTVM::vmPrintf;
    this->functions["SetBackColor"]  = &BTVM::vmSetBackColor;
    this->functions["SetForeColor"]  = &BTVM::vmSetForeColor;
    this->functions["Warning"]       = &BTVM::vmWarning;

    // I/O Functions: https://www.sweetscape.com/010editor/manual/FuncIO.htm
    this->functions["FEof"]          = &BTVM::vmFEof;
    this->functions["FileSize"]      = &BTVM::vmFileSize;
    this->functions["FTell"]         = &BTVM::vmFTell;
    this->functions["ReadBytes"]     = &BTVM::vmReadBytes;
    this->functions["ReadUInt"]      = &BTVM::vmReadUInt;
    this->functions["LittleEndian"]  = &BTVM::vmLittleEndian;
    this->functions["BigEndian"]     = &BTVM::vmBigEndian;

    // Math Functions: https://www.sweetscape.com/010editor/manual/FuncMath.htm
    this->functions["Ceil"]          = &BTVM::vmCeil;

    // Non-Standard Functions
    this->functions["__btvm_test__"] = &BTVM::vmBtvmTest; // Non-standard BTVM function for unit testing
}

void BTVM::initColors()
{
    this->_colors["cBlack"]    = 0x00000000;
    this->_colors["cRed"]      = 0x000000FF;
    this->_colors["cDkRed"]    = 0x00000080;
    this->_colors["cLtRed"]    = 0x008080FF;
    this->_colors["cGreen"]    = 0x0000FF00;
    this->_colors["cDkGreen"]  = 0x00008000;
    this->_colors["cLtGreen"]  = 0x0080FF80;
    this->_colors["cBlue"]     = 0x00FF0000;
    this->_colors["cDkBlue"]   = 0x00800000;
    this->_colors["cLtBlue"]   = 0x00FF8080;
    this->_colors["cPurple"]   = 0x00FF00FF;
    this->_colors["cDkPurple"] = 0x00800080;
    this->_colors["cLtPurple"] = 0x00FFE0FF;
    this->_colors["cAqua"]     = 0x00FFFF00;
    this->_colors["cDkAqua"]   = 0x00808000;
    this->_colors["cLtAqua"]   = 0x00FFFFE0;
    this->_colors["cYellow"]   = 0x0000FFFF;
    this->_colors["cDkYellow"] = 0x00008080;
    this->_colors["cLtYellow"] = 0x0080FFFF;
    this->_colors["cDkGray"]   = 0x00404040;
    this->_colors["cGray"]     = 0x00808080;
    this->_colors["cSilver"]   = 0x00C0C0C0;
    this->_colors["cLtGray"]   = 0x00E0E0E0;
    this->_colors["cWhite"]    = 0x00FFFFFF;
    this->_colors["cNone"]     = 0xFFFFFFFF;
}

VMValuePtr BTVM::vmPrintf(VM *self, NCall *ncall)
{
    VMValuePtr format = ncall->arguments.front()->execute(self);
    VMFunctions::ValueList args;

    if(ncall->arguments.size() > 1)
    {
        for(auto it = ncall->arguments.begin() + 1; it != ncall->arguments.end(); it++)
            args.push_back((*it)->execute(self));
    }

    static_cast<BTVM*>(self)->print(VMFunctions::format_string(format, args));
    return VMValuePtr();
}

VMValuePtr BTVM::vmSetBackColor(VM *self, NCall *ncall)
{
    if(ncall->arguments.empty() || !node_is(ncall->arguments.front(), NIdentifier))
        return VMValuePtr();

    BTVM* btvm = static_cast<BTVM*>(self);
    NIdentifier* nid = static_cast<NIdentifier*>(ncall->arguments.front());

    if(btvm->_colors.find(nid->value) == btvm->_colors.end())
        return VMValuePtr();

    btvm->setBackColor(btvm->_colors[nid->value]);
    return VMValuePtr();
}

VMValuePtr BTVM::vmSetForeColor(VM *self, NCall *ncall)
{
    if(ncall->arguments.empty())
        return VMValuePtr();

    VMValuePtr vmvalue = ncall->arguments.front()->execute(self);

    if(!vmvalue->is_string())
        return VMValuePtr();

    BTVM* btvm = static_cast<BTVM*>(self);

    if(btvm->_colors.find(vmvalue->to_string()) == btvm->_colors.end())
        return VMValuePtr();

    btvm->setForeColor(btvm->_colors[vmvalue->to_string()]);
    return VMValuePtr();
}

VMValuePtr BTVM::vmLittleEndian(VM *self, NCall *ncall)
{
    VMUnused(ncall);

    static_cast<BTVM*>(self)->_btvmio->setLittleEndian();
    return VMValuePtr();
}

VMValuePtr BTVM::vmBigEndian(VM *self, NCall *ncall)
{
    VMUnused(ncall);

    static_cast<BTVM*>(self)->_btvmio->setBigEndian();
    return VMValuePtr();
}

VMValuePtr BTVM::vmCeil(VM *self, NCall *ncall)
{
    VMValuePtr vmvalue = std::make_shared<VMValue>(*ncall->arguments.front()->execute(self));
    vmvalue->d_value = std::ceil(vmvalue->d_value);
    return vmvalue;
}

VMValuePtr BTVM::vmWarning(VM *self, NCall *ncall)
{
    static_cast<BTVM*>(self)->print("WARNING: ");
    return BTVM::vmPrintf(self, ncall);
}

VMValuePtr BTVM::vmBtvmTest(VM *self, NCall *ncall)
{
    if(ncall->arguments.size() < 1)
    {
        cout << "INVALID ARGS" << endl;
        return std::make_shared<VMValue>(false);
    }

    VMValuePtr testres = ncall->arguments.front()->execute(self);

    if(*testres)
        cout << ColorizeOk("OK") << endl;
    else
        cout << ColorizeFail("FAIL") << endl;

    return testres;
}

VMValuePtr BTVM::vmFEof(VM *self, NCall *ncall)
{
    VMUnused(ncall);
    return std::make_shared<VMValue>(static_cast<BTVM*>(self)->_btvmio->atEof());
}

VMValuePtr BTVM::vmFileSize(VM *self, NCall *ncall)
{
    VMUnused(ncall);
    return std::make_shared<VMValue>(static_cast<BTVM*>(self)->_btvmio->size());
}

VMValuePtr BTVM::vmFTell(VM *self, NCall *ncall)
{
    VMUnused(ncall);
    return std::make_shared<VMValue>(static_cast<BTVM*>(self)->_btvmio->offset());
}

VMValuePtr BTVM::vmReadBytes(VM *self, NCall *ncall)
{
    BTVM* btvm = static_cast<BTVM*>(self);

    VMValuePtr dest = ncall->arguments[0]->execute(self);
    VMValuePtr offset = ncall->arguments[1]->execute(self);
    VMValuePtr size = ncall->arguments[2]->execute(self);

    IO_NoSeek(btvm->_btvmio);

    btvm->_btvmio->seek(offset->ui_value);
    btvm->_btvmio->read(dest, size->ui_value);
    return VMValuePtr();
}

VMValuePtr BTVM::vmReadUInt(VM *self, NCall *ncall)
{
    VMValuePtr pos;
    BTVM* btvm = static_cast<BTVM*>(self);

    if(ncall->arguments.size() > 1)
        return btvm->error("Expected 0 or 1 arguments, " + std::to_string(ncall->arguments.size()) + " given");

    IO_NoSeek(btvm->_btvmio);

    if(ncall->arguments.size() == 1)
    {
        pos = ncall->arguments.front()->execute(btvm);

        if(!pos->is_integer())
            return btvm->error("Integer type expected");

        btvm->_btvmio->seek(pos->ui_value);
    }

    VMValuePtr vmvalue = std::make_shared<VMValue>(VMValue::build(32, false, false));
    btvm->_btvmio->read(vmvalue, btvm->sizeOf(vmvalue));
    return vmvalue;
}
