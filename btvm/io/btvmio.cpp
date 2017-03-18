#include "btvmio.h"
#include "../vm/ast.h"
#include <cstring>

#define BUFFER_SIZE 4096
#define align_to(x, a) (x + (a - (x % a)))

BTVMIO::BTVMIO(): _endianness(BTEndianness::PlatformEndian), _buffersize(0)
{
    this->_buffer = static_cast<uint8_t*>(malloc(BUFFER_SIZE));
}

BTVMIO::~BTVMIO()
{
    free(this->_buffer);
    this->_buffer = NULL;
}

void BTVMIO::read(const VMValuePtr &vmvalue, uint64_t bytes)
{
    if(vmvalue->value_bits != -1)
    {
        uint64_t bits = vmvalue->value_bits == -1 ? (bytes * PLATFORM_BITS) : static_cast<uint64_t>(vmvalue->value_bits);
        this->_cursor.size = bytes;
        this->readBits(vmvalue->value_ref<uint8_t>(), bits);
    }
    else
    {
        this->alignCursor();
        this->readBytes(vmvalue->value_ref<uint8_t>(), bytes);
        this->elaborateEndianness(vmvalue);
    }
}

uint64_t BTVMIO::offset() const
{
    return this->_cursor.position;
}

bool BTVMIO::atEof() const
{
    if(!this->_buffer || this->_cursor.hasBits() || !this->_cursor.moved)
        return false;

    return (this->_buffersize < BUFFER_SIZE) && (this->_cursor.rel_position >= this->_buffersize);
}

void BTVMIO::seek(uint64_t offset)
{
    this->updateBuffer();
    this->_cursor.position = offset;
}

int BTVMIO::endianness() const
{
    return this->_endianness;
}

void BTVMIO::setLittleEndian()
{
    this->_endianness = BTEndianness::LittleEndian;
}

void BTVMIO::setBigEndian()
{
    this->_endianness = BTEndianness::BigEndian;
}

uint8_t BTVMIO::readBit()
{
    uint8_t* sbuffer = this->_buffer + this->_cursor.rel_position;
    uint8_t val = (*sbuffer & (1u << this->_cursor.bit)) >> this->_cursor.bit;

    this->_cursor.bit++;

    if(this->_cursor.bit == PLATFORM_BITS)
    {
        this->_cursor++;
        this->_cursor.bit = 0;
    }

    return val;
}

uint8_t* BTVMIO::updateBuffer()
{
    this->_buffersize = this->readData(this->_buffer, BUFFER_SIZE);
    this->_cursor.rewind();
    return this->_buffer;
}

bool BTVMIO::atBufferEnd() const
{
    if(this->_cursor.hasBits())
        return false;

    return this->_cursor.rel_position >= this->_buffersize;
}

void BTVMIO::alignCursor()
{
    if(!this->_cursor.hasBits() || !this->_cursor.size)
        return;

    this->_cursor.position = align_to(this->_cursor.position, this->_cursor.size);
    this->_cursor.rel_position = align_to(this->_cursor.rel_position, this->_cursor.size);
    this->_cursor.size = this->_cursor.bit = 0;
}

void BTVMIO::readBytes(uint8_t *buffer, uint64_t bytescount)
{
    uint8_t* sbuffer = this->_buffer + this->_cursor.rel_position;
    uint8_t* dbuffer = buffer;

    for(uint64_t i = 0; i < bytescount && !this->atEof(); i++)
    {
        if(this->atBufferEnd())
            sbuffer = this->updateBuffer();

        *dbuffer = *sbuffer;
        sbuffer++; dbuffer++;

        this->_cursor++;
    }
}

void BTVMIO::readBits(uint8_t *buffer, uint64_t bitscount)
{
    uint8_t* dbuffer = buffer;

    for(uint64_t i = 0; i < bitscount && !this->atEof(); i++)
    {
        if(this->atBufferEnd())
            this->updateBuffer();

        *dbuffer |= this->readBit();

        if(!this->_cursor.bit)
        {
            dbuffer++;
            *dbuffer = 0;
        }
    }
}

void BTVMIO::elaborateEndianness(const VMValuePtr &vmvalue)
{
    if(vmvalue->value_type == VMValueType::s16)
        *vmvalue->value_ref<int64_t>() = this->cpuEndianness(*vmvalue->value_ref<int16_t>());
    else if((vmvalue->value_type == VMValueType::s32) || (vmvalue->value_type == VMValueType::Float))
        *vmvalue->value_ref<int64_t>() = this->cpuEndianness(*vmvalue->value_ref<int32_t>());
    else if((vmvalue->value_type == VMValueType::s64) || (vmvalue->value_type == VMValueType::Double))
        *vmvalue->value_ref<int64_t>() = this->cpuEndianness(*vmvalue->value_ref<int64_t>());
    else if(vmvalue->value_type == VMValueType::u16)
        *vmvalue->value_ref<uint64_t>() = this->cpuEndianness(*vmvalue->value_ref<uint16_t>());
    else if(vmvalue->value_type == VMValueType::u32)
        *vmvalue->value_ref<uint64_t>() = this->cpuEndianness(*vmvalue->value_ref<uint32_t>());
    else if(vmvalue->value_type == VMValueType::u64)
        *vmvalue->value_ref<uint64_t>() = this->cpuEndianness(*vmvalue->value_ref<uint64_t>());
}
