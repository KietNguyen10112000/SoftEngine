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

	struct FileChooserDialog
	{
		String outputFilePath;

		std::vector<ExtensionFilterGroup> extensionGroups;
		bool forceInsideResourcesPath = false;
	};

	static bool OpenSaveAsDialog(SaveAsDialog& opt);
	static bool OpenFileChooser(FileChooserDialog& opt);
};