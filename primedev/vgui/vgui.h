#pragma once

namespace vgui
{
	class Panel;
} // namespace vgui

extern vgui::Panel* (*vgui_Panel_FindChildByName)(const char* childName, bool recurseDown);
