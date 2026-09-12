#pragma once

#include "engine/ihandleentity.h"

class C_BaseEntity;
class IClientEntity;
class IClientNetworkable;
class IClientRenderable;
class IClientThinkable;
class ICollideable;

class IClientUnknown : public IHandleEntity
{
public:
	virtual ICollideable* GetCollideable() = 0; // 2
	virtual IClientNetworkable* GetClientNetworkable() = 0; // 3
	virtual IClientRenderable* GetClientRenderable() = 0; // 4
	virtual IClientEntity* GetIClientEntity() = 0; // 5
	virtual C_BaseEntity* GetBaseEntity() = 0; // 6
	virtual IClientThinkable* GetClientThinkable() = 0; // 7
};

static_assert(sizeof(IClientUnknown) == 0x10);

