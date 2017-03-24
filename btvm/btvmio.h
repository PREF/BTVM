#ifndef BTVMIO_H
#define BTVMIO_H

#include <functional>
#include "vm/vmvalue.h"
#include "vm/vm_functions.h"
#include "format/btentry.h"

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
        void readString(const VMValuePtr &vmvalue, int64_t maxlen);
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
        template<typename T> T elaborateEndianness(T valueref) const;
        void elaborateEndianness(const VMValuePtr &vmvalue) const;

    private:
        static const int PLATFORM_ENDIANNESS;
        int _platformendianness;
        int _endianness;
        BitCursor _cursor;
        uint64_t _buffersize;
        uint8_t* _buffer;
};

template<typename T> T BTVMIO::elaborateEndianness(T value) const
{
    if(this->_platformendianness == this->_endianness)
        return value;

    uint8_t *start, *end;

    for(start = reinterpret_cast<uint8_t*>(&value), end = start + sizeof(T) - 1; start < end; ++start, --end)
    {
        uint8_t swap = *start;
        *start = *end;
        *end = swap;
    }

    return value;
}
#endif // BTVMIO_H
