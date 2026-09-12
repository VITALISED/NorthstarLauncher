#pragma once

#include "engine/basehandle.h"

class IHandleEntity
{
  public:
    virtual void SetRefEHandle(const CBaseHandle& handle) = 0;
    virtual ~IHandleEntity() = default;

    const CBaseHandle& GetRefEHandle() const
    {
        return m_RefEHandle;
    }

  protected:
    CBaseHandle m_RefEHandle;
};

static_assert(sizeof(IHandleEntity) == 0x10);
