#pragma once

#include "server/baseentity.h"

class CBaseAnimating : public CBaseEntity
{
  public:
    std::byte m_Reserved09E0[0x24];
    int32_t m_camoIndex;  // 0xA04
    int32_t m_decalIndex; // 0xA08
    std::byte m_Reserved0A0C[0x4AC];
};

static_assert(sizeof(CBaseAnimating) == 0xEB8);
static_assert(offsetof(CBaseAnimating, m_camoIndex) == 0xA04);
static_assert(offsetof(CBaseAnimating, m_decalIndex) == 0xA08);
