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
	friend class Runtime;

	struct FileOrDirectory
	{
		// in ms
		size_t lastModifiedTime;

		void Serialize(ByteStream* stream);
		void Deserialize(ByteStreamRead* stream);
	};

	struct WorkingDirectory
	{
		// this must be absolute path
		String rootPath;
		std::vector<String> searchDirectories;
	};

	String m_cachePath;
	//String m_rootPath;
	//String m_rootFullPath;
	String m_executablePath;
	String m_currentPath;

	std::map<String, FileOrDirectory> m_indexedFiles;

	std::vector<WorkingDirectory> m_workingDirectories;
	bool m_isInitializedResourcePaths = false;

public:
	FileSystem();
	~FileSystem();

private:
	void LoadCache();

	inline auto GetCachePath(const String& path)
	{
		return m_cachePath + path;
	}

	void BeginInitializeResourcePaths(); 
	void EndInitializeResourcePaths();
	void SetCurrentWorkingDirectory(const String& path);

	template <typename Fn>
	void ForEachSearchDirectories(Fn fn)
	{
		for (auto& wkd : m_workingDirectories)
		{
			if (fn(wkd.rootPath, wkd.rootPath))
			{
				return;
			}

			for (auto& path : wkd.searchDirectories)
			{
				if (fn(wkd.rootPath, wkd.rootPath + path))
				{
					return;
				}
			}
		}
	}


public:
	void SaveCache();

	bool IsFileExisted(const String& relativePath);
	//bool IsResourceExist(const char* path);

	bool IsDirectoryExisted(const String& relativePath);

	// in ms
	size_t GetFileModifiedLastTime(const String& relativePath);
	bool IsFileChanged(const String& relativePath, bool updateLastModifiedTime = true);
	bool IsDirectoryChanged(const String& relativePath, bool updateLastModifiedTime = true);

	void WriteStream(const String& path, ByteStreamRead* stream);
	bool ReadStream(const String& path, ByteStream* output);

	void WriteCacheStream(const String& path, ByteStreamRead* stream);
	bool ReadCacheStream(const String& path, ByteStream* output);

	void AddWorkingDirectory(const String& workingDirectory);
	void AddSearchDirectory(const String& workingDirectory, const String& searchDirectory);

	// input: 
	//	+ path: relative path to search
	// output: 
	//	+ return the relative path to a one of working directories if found
	//	+ return empty string if path is not found
	// example:
	//	Assumption that there are 3 working directories: 1 - "D:/Engine/", 2 - "D:/MyProject/", 3 - "D:/Editor/"
	//		+ Directory 1 has a file "D:/Engine/Resources/Image/Image1.png" and AddSearchDirectory("D:/Engine/", "Resources/") called
	//			=> FindRelativeFilePath("Image/Image1.png") will return "Resources/Image/Image1.png"
	// 
	String FindRelativeFilePath(const String& relativePath, String* outputWkd = nullptr);

	//
	// same as FindAbsoluteFilePath but return a absolute path
	//
	String FindAbsoluteFilePath(const String& relativePath);

	String GetRelativeFilePath(const String& absolutePath);

	inline const String& GetCacheDirectory() const
	{
		return m_cachePath;
	}

	inline const String& GetCurrentWorkingDirectory() const
	{
		return m_currentPath;
	}

	inline const String& GetExecutableDirectory() const
	{
		return m_executablePath;
	}

	template <typename Fn>
	void ForEachWorkingDirectory(Fn fn)
	{
		for (auto& wkd : m_workingDirectories)
		{
			fn(wkd.rootPath);
		}
	}
};

NAMESPACE_END