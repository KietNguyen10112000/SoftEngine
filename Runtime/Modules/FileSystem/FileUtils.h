#pragma once

#include "TypeDef.h"

#include "Core/Memory/Memory.h"
#include "Core/Structures/String.h"

#include <stdio.h>
#include <filesystem>

namespace std 
{
	namespace fs = filesystem;
}

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
	
	auto buf = (byte*)rheap::malloc(std::max(64 * KB, size));

	fseek(fp, 0L, SEEK_SET);
	fread(buf, size, sizeof(byte), fp);
	//t.seekg(0);
	//t.read((char*)&buf[0], size);

	fclose(fp);

	buffer = buf;
	fileSize = size;
}


inline void WriteFile(const char* fileName, const void* buffer, size_t bufferSize)
{
	FILE* fp = fopen(fileName, "wb");

	fwrite(buffer, sizeof(byte), bufferSize, fp);

	fclose(fp);
}


inline void FreeBuffer(byte*& buffer)
{
	rheap::free(buffer);
	buffer = nullptr;
}

// no recursive
template <typename Func>
inline void ForEachFiles(String path, Func callback)
{
	for (const auto& entry : std::fs::directory_iterator(path.c_str()))
	{
		if (entry.is_regular_file())
		{
			callback(entry.path().c_str());
		}
	}
}

inline bool IsExist(const char* path)
{
	return std::fs::exists(path);
}

}

NAMESPACE_FILE_SYSTEM_END