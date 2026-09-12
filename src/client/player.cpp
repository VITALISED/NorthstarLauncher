#include "client/player.h"
#include "tier0/callbacks.h"

using CPlayerIsMantlingFn = bool (*)(const C_Player*);
static CPlayerIsMantlingFn s_CPlayerIsMantling;

bool C_Player::IsMantling() const
{
	return s_CPlayerIsMantling(this);
}

ON_DLL_LOAD_CLIENT("client.dll", ClientPlayerMethods, [](CModule module)
{
	s_CPlayerIsMantling = module.Offset(0x9E0B0).RCast<CPlayerIsMantlingFn>();
})
