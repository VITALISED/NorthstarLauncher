#pragma once

#include "server/iserverunknown.h"

class IServerEntity : public IServerUnknown
{
};

static_assert(sizeof(IServerEntity) == 0x10);
