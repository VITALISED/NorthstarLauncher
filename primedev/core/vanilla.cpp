#include "core/vanilla.h"
#include "engine/r2engine.h"

VanillaCompatibility* g_pVanillaCompatibility;

ConVar* Cvar_ns_skip_vanilla_integrity_check;

bool VanillaCompatibility::GetVanillaCompatibility()
{
	if (g_pCVar->FindVar("serverfilter")->GetBool() && !g_pCVar->FindVar("ns_is_northstar_server")->GetBool() &&
		!g_pCVar->FindVar("ns_auth_allow_insecure")->GetBool())
	{
		// replicate old behaviour
		if (Cvar_ns_skip_vanilla_integrity_check->GetBool())
			return false;

		// directly call _Cmd_Exec_f to avoid weird cbuf crashes
		char* commandBuf[1040];
		memset(commandBuf, 0, sizeof(commandBuf));
		CCommand tempCommand = *(CCommand*)&commandBuf;
		CCommand__Tokenize(tempCommand, "disconnect \"Server outdated or evil!\"", cmd_source_t::kCommandSrcCode);
		_Cmd_Exec_f(tempCommand, false, false);

		return false;
	}

	return g_pCVar->FindVar("ns_is_northstar_server")->GetBool() == 0;
}

ON_DLL_LOAD_RELIESON("engine.dll", VanillaCompat, ConVar, (CModule module))
{
	Cvar_ns_skip_vanilla_integrity_check =
		new ConVar("ns_skip_vanilla_integrity_check", "0", FCVAR_NONE, "Skip vanilla integrity check for compatibility with older servers");
}
