#pragma once

#include <cstdint>

class IClientUnknown;
using ClientThinkHandle_t = std::uint16_t;

class IClientThinkable
{
public:
	virtual IClientUnknown* GetIClientUnknown() = 0; // 0
	virtual void ClientThink() = 0; // 1
	virtual ClientThinkHandle_t GetThinkHandle() = 0; // 2
	virtual void SetThinkHandle(ClientThinkHandle_t handle) = 0; // 3
	virtual void Release() = 0; // 4
};

static_assert(sizeof(IClientThinkable) == sizeof(void*));
