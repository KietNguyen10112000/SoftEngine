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
		void Deserialize(ByteStreamRead* stream);
	};

	String m_cachePath;
	String m_rootPath;
	String m_rootFullPath;
	String m_executablePath;

	std::map<String, FileOrDirectory> m_indexedFiles;

public:
	FileSystem();
	~FileSystem();

private:
	void LoadCache();

	inline auto GetCachePath(const String& path)
	{
		return m_cachePath + path;
	}

	// get full resource path
	inline auto GetFullPath(const String& path)
	{
		std::string_view str = path.c_str();
		if (str.find_first_of(m_rootPath.c_str()) == 0)
		{
			return String(path);
		}
		return m_rootPath + path;
	}

public:
	void SaveCache();

	bool IsFileExist(const char* path);
	bool IsResourceExist(const char* path);

	bool IsDirectoryExist(const char* path);

	// in ms
	size_t GetFileModifiedLastTime(const String& path) const;
	bool IsFileChanged(const char* path, bool updateLastModifiedTime = true);
	bool IsDirectoryChanged(const char* path, bool updateLastModifiedTime = true);

	void WriteStream(const String& path, ByteStreamRead* stream);
	bool ReadStream(const String& path, ByteStream* output);

	void WriteCacheStream(const String& path, ByteStreamRead* stream);
	bool ReadCacheStream(const String& path, ByteStream* output);

	// full path to Resources/
	String GetResourcesRootPath();

	String GetResourcesPath(String path)
	{
		return m_rootFullPath + path;
	}

	String GetResourcesRelativePath(String path)
	{
		return path;
	}

	inline const String& GetExecutablePath() const
	{
		return m_executablePath;
	}

	inline const String& GetCachePath() const
	{
		return m_cachePath;
	}
};

NAMESPACE_END