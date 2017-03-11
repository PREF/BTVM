#ifndef BTENTRY_H
#define BTENTRY_H

#include <memory>
#include <string>
#include <vector>
#include "../vm/vmvalue.h"

enum BTEndianness
{
    PlatformEndian = 0,
    LittleEndian,
    BigEndian,
};

struct BTLocation
{
    BTLocation(): offset(0), size(0) { }
    BTLocation(uint64_t offset, uint64_t size): offset(offset), size(size) { }

    uint64_t offset;
    uint64_t size;
};

struct BTEntry
{
    BTEntry() { }
    BTEntry(const std::string& name): name(name), endianness(BTEndianness::PlatformEndian), backcolor(0xFFFFFFFF), forecolor(0xFFFFFFFF) { }
    BTEntry(const VMValuePtr& value, uint32_t endianness): name(value->value_id), value(value), endianness(endianness), backcolor(0xFFFFFFFF), forecolor(0xFFFFFFFF) { }

    std::string name;
    VMValuePtr value;
    BTLocation location;
    uint32_t endianness;
    uint32_t backcolor;
    uint32_t forecolor;
    std::vector< std::shared_ptr<BTEntry> > children;
};

typedef std::shared_ptr<BTEntry> BTEntryPtr;
typedef std::vector<BTEntryPtr> BTEntryList;


#endif // BTENTRY_H
