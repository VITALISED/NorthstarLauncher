#pragma once

#include "server/baseanimating.h"

class CBaseAnimatingOverlay : public CBaseAnimating
{
  protected:
    std::byte m_Reserved0EB8[0x188];
};

static_assert(sizeof(CBaseAnimatingOverlay) == 0x1040);
