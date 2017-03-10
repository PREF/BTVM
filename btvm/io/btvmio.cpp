#include "btvmio.h"
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
        else if(btv->value_type == VMValueType::s8)
            btv->si_value = *reinterpret_cast<int8_t*>(buffer);
        else if(btv->value_type == VMValueType::s16)
            btv->si_value = this->cpuEndianness(*reinterpret_cast<int16_t*>(buffer));
        else if(btv->value_type == VMValueType::s32)
            btv->si_value = this->cpuEndianness(*reinterpret_cast<int32_t*>(buffer));
        else if(btv->value_type == VMValueType::s64)
            btv->si_value = this->cpuEndianness(*reinterpret_cast<int64_t*>(buffer));
        else if(btv->value_type == VMValueType::u8)
            btv->ui_value = this->cpuEndianness(*reinterpret_cast<uint8_t*>(buffer));
        else if(btv->value_type == VMValueType::u16)
            btv->ui_value = this->cpuEndianness(*reinterpret_cast<uint16_t*>(buffer));
        else if(btv->value_type == VMValueType::u32)
            btv->ui_value = this->cpuEndianness(*reinterpret_cast<uint32_t*>(buffer));
        else //if(btv->value_type == VMValueType::u64)
            btv->ui_value = this->cpuEndianness(*reinterpret_cast<uint64_t*>(buffer));
    }
    else if(btv->is_floating_point())
    {
        if(btv->value_type == VMValueType::Float)
            btv->d_value = *reinterpret_cast<float*>(buffer);
        else
            btv->d_value = *reinterpret_cast<double*>(buffer);
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
