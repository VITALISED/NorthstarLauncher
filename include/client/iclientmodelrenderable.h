#pragma once

class IClientModelRenderable
{
private:
	virtual void UnknownModelRenderable000() = 0;
};

static_assert(sizeof(IClientModelRenderable) == sizeof(void*));
