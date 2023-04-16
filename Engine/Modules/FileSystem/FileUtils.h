#pragma once

#include "TypeDef.h"

#include "Core/Memory/Memory.h"
#include "Core/Structures/String.h"

#include <stdio.h>

NAMESPACE_FILE_SYSTEM_BEGIN

namespace FileUtils
{

inline void ReadFile(String fileName, byte*& buffer, size_t& fileSize)
{
	//std::ifstream t(fileName.c_str(), std::ios::binary);
	//t.seekg(0, std::ios::end);
	//size_t size = t.tellg();

	// C++ ifstream does something magic in release mode
	// so custom memory allocator doesn't work
	// using C instead

	FILE* fp = fopen(fileName.c_str(), "rb");
	fseek(fp, 0L, SEEK_END);
	size_t size = ftell(fp);
	
	auto buf = rheap::NewArray<byte>(size);

	fseek(fp, 0L, SEEK_SET);
	fread(buf, size, sizeof(byte), fp);
	//t.seekg(0);
	//t.read((char*)&buf[0], size);

	fclose(fp);

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