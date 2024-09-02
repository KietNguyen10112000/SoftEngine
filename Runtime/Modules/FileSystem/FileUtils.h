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

	assert(!fileName.empty() && std::filesystem::exists(fileName.c_str()));

	if (!std::filesystem::exists(fileName.c_str()))
	{
		std::cerr << "File: " << fileName << "doesn't exist!\n";
	}

	FILE* fp = fopen(fileName.c_str(), "rb");
	fseek(fp, 0L, SEEK_END);
	size_t size = ftell(fp);
	
	auto buf = (byte*)rheap::malloc(std::max(64 * KB, size + 1));

	fseek(fp, 0L, SEEK_SET);
	fread(buf, size, sizeof(byte), fp);
	//t.seekg(0);
	//t.read((char*)&buf[0], size);

	fclose(fp);

	buffer = buf;
	fileSize = size;
	buffer[fileSize] = '\0';
}


inline void WriteFile(const char* fileName, const void* buffer, size_t bufferSize)
{
	FILE* fp = fopen(fileName, "wb");

	if (!fp)
	{
		std::filesystem::path fspath{ fileName };
		std::filesystem::create_directories(fspath.parent_path());
		fp = fopen(fileName, "wb+");
	}

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

//inline bool IsExist(const char* path)
//{
//	return std::fs::exists(path);
//}

inline String GetLastName(const char* path)
{
	std::string_view str = path;
	auto idx = str.find_last_of('/');
	if (idx == std::string_view::npos)
	{
		return path;
	}

	idx++;

	return String(path + idx, str.length() - idx);
}

inline String PopPath(const String& path)
{
	assert(path.length() >= 1);

	std::string_view str = std::string_view(path.c_str(), path[path.length() - 1] == '/' ? path.length() - 1 : path.length());
	auto idx = str.rfind('/');
	if (idx == std::string_view::npos)
	{
		return path;
	}

	idx++;

	return String(path.c_str(), idx);
}

inline String ShiftPath(const String& path)
{
	std::string_view str = path.c_str();
	auto idx = str.find_first_of('/');
	if (idx == std::string_view::npos)
	{
		return path;
	}

	idx++;

	return String(path.c_str() + idx);
}

inline String GetExtension(const String& path)
{
	return path.SubString(path.FindLastOf(".") + 1);
}

inline String JoinPaths(String base, String relative)
{
	String pattern = "../";
	while (!relative.empty())
	{
		auto idx = relative.Find('/');
		if (idx == INVALID_ID)
		{
			break;
		}

		auto head = relative.SubString(0, idx + 1);
		if (head != pattern)
		{
			break;
		}

		base = PopPath(base);
		relative = ShiftPath(relative);
	}

	return base + relative;
}

}

NAMESPACE_FILE_SYSTEM_END