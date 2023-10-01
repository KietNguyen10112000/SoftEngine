#pragma once

#include "Core/TypeDef.h"
#include "Core/Pattern/Singleton.h"
#include "Core/Structures/String.h"

#include "Runtime/Common/Stream/ByteStream.h"

#include <map>
#include <filesystem>

NAMESPACE_BEGIN

class API FileSystem : public Singleton<FileSystem>
{
public:
	static void Initialize();
	static void Finalize();

private:
	friend class ByteStream;

	struct FileOrDirectory
	{
		// in ms
		size_t lastModifiedTime;

		void Serialize(ByteStream* stream);
		void Deserialize(ByteStream* stream);
	};

	String m_cachePath;
	String m_rootPath;

	std::map<String, FileOrDirectory> m_indexedFiles;

public:
	FileSystem();
	~FileSystem();

private:
	void LoadCache();
	void SaveCache();

	inline auto GetCachePath(const char* path)
	{
		return m_cachePath + path;
	}

	inline auto GetFullPath(const char* path)
	{
		std::string_view str = path;
		if (str.find_first_of(m_rootPath.c_str()) == 0)
		{
			return String(path);
		}
		return m_rootPath + path;
	}

public:
	inline bool IsFileExist(const char* path)
	{
		return std::filesystem::exists(path);
	}

	inline bool IsDirectoryExist(const char* path)
	{
		return std::filesystem::exists(path);
	}

	bool IsFileChanged(const char* path, bool updateLastModifiedTime = true);
	bool IsDirectoryChanged(const char* path, bool updateLastModifiedTime = true);

	void WriteStream(const char* path, ByteStreamRead* stream);
	bool ReadStream(const char* path, ByteStream* output);
};

NAMESPACE_END