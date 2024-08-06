#pragma once

#include "Core/Structures/String.h"

using namespace soft;

#include <vector>

struct SystemDialog
{
	struct ExtensionFilterGroup
	{
		String groupName;
		std::vector<String> extensions;
	};

	struct SaveAsDialog
	{
		String outputFilePath;
		String defaultPath;

		std::vector<ExtensionFilterGroup> extensionGroups;
	};

	static bool OpenSaveAsDialog(SaveAsDialog& opt);
};