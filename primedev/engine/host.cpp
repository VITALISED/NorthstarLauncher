#include "core/convar/convar.h"
#include "core/tier0.h"
#include "mods/modmanager.h"
#include "r2engine.h"
#include "shared/misccommands.h"
#include "util/printcommands.h"
#include "util/printmaps.h"

AUTOHOOK_INIT()

// clang-format off
AUTOHOOK(Host_Init, engine.dll + 0x155EA0,
void, __fastcall, (bool bDedicated))
// clang-format on
{
	spdlog::info("Host_Init()");
	Host_Init(bDedicated);
	FixupCvarFlags();
	// need to initialise these after host_init since they do stuff to preexisting concommands/convars without being client/server specific
	InitialiseCommandPrint();
	InitialiseMapsPrint();
	// client/server autoexecs on necessary platforms
	// dedi needs autoexec_ns_server on boot, while non-dedi will run it on on listen server start
	if (bDedicated)
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec autoexec_ns_dedicatedserver", cmd_source_t::kCommandSrcCode);
	else
	{
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec autoexec_ns_client", cmd_source_t::kCommandSrcCode);
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec autoexec_ns_listenserver", cmd_source_t::kCommandSrcCode);
	}
}

ON_DLL_LOAD("engine.dll", Host_Init, (CModule module))
{
	AUTOHOOK_DISPATCH()
}
