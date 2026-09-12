#pragma once

#include "engine/ihandleentity.h"

class CBaseEntity;

class IServerUnknown : public IHandleEntity
{
  public:
    virtual CBaseEntity* GetMoveParent() = 0;
};

static_assert(sizeof(IServerUnknown) == 0x10);
