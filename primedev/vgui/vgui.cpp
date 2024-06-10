#include "vgui/vgui.h"

vgui::Panel* (*vgui_Panel_FindChildByName)(const char* childName, bool recurseDown);

ON_DLL_LOAD_CLIENT("client.dll", VGuiOffsets, (CModule module))
{
	vgui_Panel_FindChildByName = module.Offset(0x759F50).RCast<vgui::Panel* (*)(const char*, bool)>();
}
