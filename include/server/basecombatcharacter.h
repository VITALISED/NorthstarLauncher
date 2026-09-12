#pragma once

#include "server/baseanimatingoverlay.h"

class CBaseCombatCharacter : public CBaseAnimatingOverlay
{
  public:
    CBaseEntity* GetActiveWeapon();
    bool IsWeaponHolstered();
    void HolsterWeapon();

    std::byte m_Reserved1040[0x314];
    int32_t m_selectedOffhand;                    // 0x1354
    int32_t m_selectedOffhandPendingHybridAction; // 0x1358
    char _unk_0x135c[92];
    int32_t m_titanSoul; // 0x13B8
    std::byte m_Reserved13BC[0xDC];
};

static_assert(sizeof(CBaseCombatCharacter) == 0x1498);
static_assert(offsetof(CBaseCombatCharacter, m_selectedOffhand) == 0x1354);
static_assert(offsetof(CBaseCombatCharacter, m_selectedOffhandPendingHybridAction) == 0x1358);
static_assert(offsetof(CBaseCombatCharacter, m_titanSoul) == 0x13B8);
