#ifndef BTVMIO_H
#define BTVMIO_H

#include "../vm/vmvalue.h"
#include "../vm/vm_functions.h"
#include "../format/btentry.h"

#define IO_NoSeek(btvmio) BTVMIO::NoSeek __noseek__(btvmio)

class BTVMIO
{
    public:
        struct NoSeek {
            NoSeek(BTVMIO* btvmio): _btvmio(btvmio), _oldoffset(btvmio->offset()) {  }
            ~NoSeek() { _btvmio->seek(_oldoffset); }

            private:
                BTVMIO* _btvmio;
                uint64_t _oldoffset;
        };

    public:
        BTVMIO();
        virtual ~BTVMIO();
        void read(const VMValuePtr &vmvalue, uint64_t size);

    public:
        virtual uint64_t offset() const = 0;
        virtual bool atEof() const = 0;
        virtual void seek(uint64_t offset) = 0;
        virtual uint64_t size() const = 0;

    public:
        int endianness() const;
        void setLittleEndian();
        void setBigEndian();

    protected:
        virtual void readData(uint8_t* buffer, uint64_t size) = 0;

    private:
        template<typename T> T cpuEndianness(T value) const;
        void readInteger(const VMValuePtr& vmvalue, const uint8_t *buffer, uint64_t size, bool issigned);

    private:
        int _endianness;
};

template<typename T> T BTVMIO::cpuEndianness(T value) const
{
    T cpuvalue = 0;

    if(this->_endianness == BTEndianness::LittleEndian) // CPU -> LE
    {
        for(size_t i = 0; i < sizeof(T); i++)
        {
            uint8_t b = static_cast<uint8_t>(value >> (i * BITS));
            cpuvalue |= (static_cast<T>(b) << (i * BITS));
        }
    }
    else // CPU -> BE
    {
        for(size_t i = sizeof(T); i > 0; i--)
        {
            uint8_t b = static_cast<uint8_t>(value >> (i * BITS));
            cpuvalue |= (static_cast<T>(b) << (i * BITS));
        }
    }

    return cpuvalue;
}
#endif // BTVMIO_H
