#pragma once

#include <datamap.h>

struct ServerDataMap;

struct ServerTypeDescription
{
    fieldtype_t fieldType;
    const char* fieldName;
    std::int32_t fieldOffset;
    std::uint16_t fieldSize;
    std::int16_t flags;
    const char* externalName;
    ISaveRestoreOps* pSaveRestoreOps;
    std::byte inputFunc[0x8];
    ServerDataMap* td;
    std::int32_t fieldSizeInBytes;
    std::uint32_t reserved3C;
    std::int32_t fieldAlignment;
    ServerTypeDescription* override_field;
    std::int32_t override_count;
    float fieldTolerance;
    std::int32_t flatOffset[TD_OFFSET_COUNT];
    std::uint16_t flatGroup;
    std::byte reserved62[0x6];
};

struct ServerDataMap
{
    ServerTypeDescription* dataDesc;
    std::int32_t dataNumFields;
    const char* dataClassName;
    std::int32_t dataSize;
    std::int32_t dataAlignment;
    std::uint64_t reserved20;
    ServerDataMap* baseMap;
};

static_assert(sizeof(ServerTypeDescription) == 0x68);
static_assert(alignof(ServerTypeDescription) == 0x8);
static_assert(offsetof(ServerTypeDescription, fieldName) == 0x8);
static_assert(offsetof(ServerTypeDescription, fieldOffset) == 0x10);
static_assert(offsetof(ServerTypeDescription, flags) == 0x16);
static_assert(offsetof(ServerTypeDescription, externalName) == 0x18);
static_assert(offsetof(ServerTypeDescription, inputFunc) == 0x28);
static_assert(offsetof(ServerTypeDescription, td) == 0x30);
static_assert(offsetof(ServerTypeDescription, fieldSizeInBytes) == 0x38);
static_assert(offsetof(ServerTypeDescription, fieldAlignment) == 0x40);
static_assert(offsetof(ServerTypeDescription, override_field) == 0x48);
static_assert(offsetof(ServerTypeDescription, flatOffset) == 0x58);
static_assert(offsetof(ServerTypeDescription, flatGroup) == 0x60);
static_assert(sizeof(ServerDataMap) == 0x30);
static_assert(offsetof(ServerDataMap, dataNumFields) == 0x8);
static_assert(offsetof(ServerDataMap, dataClassName) == 0x10);
static_assert(offsetof(ServerDataMap, baseMap) == 0x28);
