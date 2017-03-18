#ifndef BTVMIO_H
#define BTVMIO_H

#include <functional>
#include "../vm/vmvalue.h"
#include "../vm/vm_functions.h"
#include "../format/btentry.h"

#define IO_NoSeek(btvmio) BTVMIO::NoSeek __noseek__(btvmio)

class BTVMIO
{
    private:
        struct BitCursor {
            BitCursor(): position(0), rel_position(0), bit(0), size(0), moved(false) { }
            void rewind() { rel_position = bit = size = 0; moved = true; }
            bool hasBits() const { return bit > 0; }
            BitCursor& operator++(int) { position++; rel_position++; moved = true; return *this; }
            uint64_t position, rel_position, bit, size;
            bool moved;
        };

    public:
        struct NoSeek {
            NoSeek(BTVMIO* btvmio): _btvmio(btvmio), _oldcursor(_btvmio->_cursor) { }
            ~NoSeek() { _btvmio->seek(_oldcursor.position); }

            private:
                BTVMIO* _btvmio;
                BitCursor _oldcursor;
        };

    public:
        BTVMIO();
        virtual ~BTVMIO();
        void read(const VMValuePtr &vmvalue, uint64_t bytes);
        void walk(uint64_t steps);
        uint64_t offset() const;
        bool atEof() const;

    public:
        virtual void seek(uint64_t offset);
        virtual uint64_t size() const = 0;

    public:
        int endianness() const;
        void setLittleEndian();
        void setBigEndian();

    private:
        uint8_t readBit();
        uint8_t *updateBuffer();
        bool atBufferEnd() const;
        void alignCursor();
        void readBytes(uint8_t* buffer, uint64_t bytescount);
        void readBits(uint8_t* buffer, uint64_t bitscount);

    protected:
        virtual uint64_t readData(uint8_t* buffer, uint64_t size) = 0;

    private:
        template<typename T> T cpuEndianness(T value) const;
        void elaborateEndianness(const VMValuePtr &vmvalue);

    private:
        int _endianness;
        BitCursor _cursor;
        uint64_t _buffersize;
        uint8_t* _buffer;
};

template<typename T> T BTVMIO::cpuEndianness(T value) const
{
    T cpuvalue = 0;
    uint8_t* pval = reinterpret_cast<uint8_t*>(&cpuvalue);

    if(this->_endianness == BTEndianness::LittleEndian) // CPU -> LE
    {
        for(size_t i = 0; i < sizeof(T); i++)
        {
            T shift = i * PLATFORM_BITS, mask = 0xFF << shift;
            pval[i] = (value & mask) >> shift;
        }
    }
    else if(this->_endianness == BTEndianness::BigEndian) // CPU -> BE
    {
        for(size_t i = 0, j = sizeof(T) - 1; i < sizeof(T); i++, j--)
        {
            T shift = j * PLATFORM_BITS, mask = 0xFF << shift;
            pval[i] = (value & mask) >> shift;
        }
    }
    else
        return value;

    return cpuvalue;
}
#endif // BTVMIO_H
