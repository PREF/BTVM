#include "btvmio.h"
#include "../vm/ast.h"
#include <cstring>

BTVMIO::BTVMIO(): _endianness(BTEndianness::PlatformEndian)
{

}

BTVMIO::~BTVMIO()
{

}

void BTVMIO::read(const VMValuePtr &vmvalue, uint64_t size)
{
    uint8_t* buffer = static_cast<uint8_t*>(malloc(size));
    this->readData(buffer, size);

    if(vmvalue->is_integer())
    {
        if(vmvalue->value_type == VMValueType::Bool)
            *vmvalue->value_ref<bool>() = (*reinterpret_cast<bool*>(buffer) != 0);
        else
            this->readInteger(vmvalue, buffer, size, vmvalue->is_signed());
    }
    else if(vmvalue->is_floating_point())
    {
        if(vmvalue->value_type == VMValueType::Float)
            *vmvalue->value_ref<float>() = *reinterpret_cast<float*>(buffer);
        else
            *vmvalue->value_ref<double>() = *reinterpret_cast<float*>(buffer);
    }
    else if(node_is(vmvalue->value_typedef, NEnum))
    {
        NEnum* nenum = static_cast<NEnum*>(vmvalue->value_typedef);

        if(node_inherits(nenum->type, NScalarType))
        {
            NScalarType* nscalartype = static_cast<NScalarType*>(nenum->type);
            this->readInteger(vmvalue, buffer, size, nscalartype->is_signed);
        }
    }
    else
        std::memcpy(vmvalue->value_ref<char>(), buffer, size);

    free(buffer);
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

void BTVMIO::readInteger(const VMValuePtr &vmvalue, const uint8_t *buffer, uint64_t size, bool issigned)
{
    if(issigned)
    {
        if(size <= 1)
            *vmvalue->value_ref<int64_t>() = *buffer;
         else if(size <= 2)
            *vmvalue->value_ref<int64_t>() = this->cpuEndianness(*reinterpret_cast<const int16_t*>(buffer));
         else if(size <= 4)
            *vmvalue->value_ref<int64_t>() = this->cpuEndianness(*reinterpret_cast<const int32_t*>(buffer));
         else
            *vmvalue->value_ref<int64_t>() = this->cpuEndianness(*reinterpret_cast<const int64_t*>(buffer));

         return;
    }

    if(size <= 1)
        *vmvalue->value_ref<uint64_t>() = *buffer;
    else if(size <= 2)
        *vmvalue->value_ref<uint64_t>() = this->cpuEndianness(*reinterpret_cast<const uint16_t*>(buffer));
    else if(size <= 4)
        *vmvalue->value_ref<uint64_t>() = this->cpuEndianness(*reinterpret_cast<const uint32_t*>(buffer));
    else
        *vmvalue->value_ref<uint64_t>() = this->cpuEndianness(*reinterpret_cast<const uint64_t*>(buffer));
}
