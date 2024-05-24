#pragma once

#include "Core/Structures/String.h"

using namespace soft;

class FileChooser
{
public:
	static String OpenFileChooser(const String& filterExtensions, bool allowOutsideResources);
};