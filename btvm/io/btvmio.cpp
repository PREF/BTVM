#include "btvmio.h"
#include "../vm/ast/ast.h"
#include <cstring>

BTVMIO::BTVMIO(): _endianness(BTEndianness::PlatformEndian)
{

}

BTVMIO::~BTVMIO()
{

}

void BTVMIO::read(const VMValuePtr &btv, uint64_t size)
{
    if(btv->is_array())
    {
        for(auto it = btv->m_value.begin(); it != btv->m_value.end(); it++)
            this->read(*it, size);

        return;
    }

    uint8_t* buffer = static_cast<uint8_t*>(malloc(size));
    this->readData(buffer, size);

    if(btv->is_integer())
    {
        if(btv->value_type == VMValueType::Bool)
            btv->ui_value = (*reinterpret_cast<bool*>(buffer) != 0);
        else
            this->readInteger(btv, buffer, size, btv->is_signed());
    }
    else if(btv->is_floating_point())
    {
        if(btv->value_type == VMValueType::Float)
            btv->d_value = *reinterpret_cast<float*>(buffer);
        else
            btv->d_value = *reinterpret_cast<double*>(buffer);
    }
    else if(node_is(btv->n_value, NEnum))
    {
        NEnum* nenum = static_cast<NEnum*>(btv->n_value);

        if(node_inherits(nenum->type, NScalarType))
        {
            NScalarType* nscalartype = static_cast<NScalarType*>(nenum->type);
            this->readInteger(btv, buffer, size, nscalartype->is_signed);
        }
    }

    else if(btv->is_string())
        std::memcpy(btv->s_value.data(), buffer, size);

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

void BTVMIO::readInteger(const VMValuePtr &btv, const uint8_t *buffer, uint64_t size, bool issigned)
{
    if(issigned)
    {
        if(size <= 1)
            btv->si_value = *buffer;
         else if(size <= 2)
            btv->si_value = this->cpuEndianness(*reinterpret_cast<const int16_t*>(buffer));
         else if(size <= 4)
            btv->si_value = this->cpuEndianness(*reinterpret_cast<const int32_t*>(buffer));
         else
            btv->si_value = this->cpuEndianness(*reinterpret_cast<const int64_t*>(buffer));

         return;
    }

    if(size <= 1)
        btv->ui_value = *buffer;
    else if(size <= 2)
        btv->ui_value = this->cpuEndianness(*reinterpret_cast<const uint16_t*>(buffer));
    else if(size <= 4)
        btv->ui_value = this->cpuEndianness(*reinterpret_cast<const uint32_t*>(buffer));
    else
        btv->ui_value = this->cpuEndianness(*reinterpret_cast<const uint64_t*>(buffer));
}
