#ifndef BTENTRY_H
#define BTENTRY_H

#include <memory>
#include <string>
#include <vector>
#include "../vm/vmvalue.h"

enum BTEndianness
{
    LittleEndian,
    BigEndian,
};

struct BTLocation
{
    BTLocation(): offset(0), size(0) { }
    BTLocation(uint64_t offset, uint64_t size): offset(offset), size(size) { }
    uint64_t end() const { return (offset + size) - 1; }

    uint64_t offset;
    uint64_t size;
};

class BTEntry;

typedef std::shared_ptr<BTEntry> BTEntryPtr;
typedef std::vector<BTEntryPtr> BTEntryList;

struct BTEntry
{
    BTEntry() { }
    BTEntry(const VMValuePtr& value, size_t endianness): name(value->value_id), value(value), endianness(endianness), backcolor(0xFFFFFFFF), forecolor(0xFFFFFFFF) { }

    std::string name;
    VMValuePtr value;
    BTLocation location;
    size_t endianness;
    uint32_t backcolor;
    uint32_t forecolor;
    BTEntryPtr parent;
    BTEntryList children;
};


#endif // BTENTRY_H
