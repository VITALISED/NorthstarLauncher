#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// An empty extension indexes every file in the archive.
bool VPKDirectory_GetFileList(
	const std::filesystem::path& directoryFile, std::string_view extension, std::vector<std::string>& entries);
