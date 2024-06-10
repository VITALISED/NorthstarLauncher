#include "vgui/basemodui.h"

AUTOHOOK_INIT()

FUNCTION_AT(client.dll + 0x4CD950, void, __fastcall, BaseModUI__LoadingProgress__DrawLoadingBar, (__int64 thisptr));

// clang-format off
AUTOHOOK(BaseModUI__LoadingProgress__PaintBackground, client.dll + 0x4CEB20,
void, __fastcall, (__int64 thisptr))
// clang-format on
{
	BaseModUI__LoadingProgress__PaintBackground(thisptr);
	BaseModUI__LoadingProgress__DrawLoadingBar(thisptr);
}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", BaseModUIHooks, ConVar, (CModule module))
{
	AUTOHOOK_DISPATCH()
}
