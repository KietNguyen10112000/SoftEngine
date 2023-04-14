#pragma once

#include "TypeDef.h"

#include "Core/Memory/Memory.h"
#include "Core/Structures/String.h"

#include <fstream>

NAMESPACE_FILE_SYSTEM_BEGIN

namespace FileUtils
{

inline void ReadFile(String fileName, byte*& buffer, size_t& fileSize)
{
	std::ifstream t(fileName.c_str(), std::ios::binary);
	t.seekg(0, std::ios::end);
	size_t size = t.tellg();
	
	auto buf = rheap::NewArray<byte>(size);

	t.seekg(0);
	t.read((char*)&buf[0], size);

	buffer = buf;
	fileSize = size;
}

inline void FreeBuffer(byte*& buffer)
{
	rheap::Delete(buffer);
	buffer = nullptr;
}

}

NAMESPACE_FILE_SYSTEM_END