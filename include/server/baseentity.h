#pragma once

#include <cstddef>
#include <cstdint>

#include "server/iserverentity.h"
#include "mathlib/vector.h"
#include "tier0/tier0_iface.h"

class CBaseEntity : public IServerEntity
{
  public:
    bool IsPlayer() const
    {
        return CallVFunc<bool>(83, this);
    }

    std::byte m_Reserved0010[0x48];
    uint32_t m_nPlayerIndex; // 0x58
    char _unk_0x5c[572];
    int32_t m_fFlags; // 0x298
    char _unk_0x29c[376];
    int32_t m_hGroundEntity; // 0x414
    char _unk_0x418[120];
    Vector3 m_vecAbsOrigin; // 0x490
    char _unk_0x49c[52];
    int32_t m_iMaxHealth; // 0x4D0
    int32_t m_iHealth;    // 0x4D4
    char _unk_0x4d8[25];
    std::uint8_t m_lifeState; // 0x4F1
    char _unk_0x4f2[26];
    float m_flMaxspeed; // 0x50C
    std::byte m_Reserved0510[0x4D0];
};

static_assert(sizeof(CBaseEntity) == 0x9E0);
static_assert(alignof(CBaseEntity) == 0x8);
static_assert(offsetof(CBaseEntity, m_nPlayerIndex) == 0x58);
static_assert(offsetof(CBaseEntity, m_fFlags) == 0x298);
static_assert(offsetof(CBaseEntity, m_hGroundEntity) == 0x414);
static_assert(offsetof(CBaseEntity, m_vecAbsOrigin) == 0x490);
static_assert(offsetof(CBaseEntity, m_iHealth) == 0x4D4);
static_assert(offsetof(CBaseEntity, m_lifeState) == 0x4F1);
static_assert(offsetof(CBaseEntity, m_flMaxspeed) == 0x50C);
