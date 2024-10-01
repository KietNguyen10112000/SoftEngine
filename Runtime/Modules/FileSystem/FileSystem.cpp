#include "FileSystem.h"

#include "Core/Memory/NewMalloc.h"

#include "Runtime/StartupConfig.h"

#include "Platform/Platform.h"

#include "FileUtils.h"

#include <cassert>

namespace fs = std::filesystem;

NAMESPACE_BEGIN

void FileSystem::Initialize()
{
	s_instance.reset(NewMalloc<FileSystem>());
}

void FileSystem::Finalize()
{
	DeleteMalloc((FileSystem*)s_instance.release());
}

void FileSystem::FileOrDirectory::Serialize(ByteStream* stream)
{
	stream->Put(lastModifiedTime);
}

void FileSystem::FileOrDirectory::Deserialize(ByteStreamRead* stream)
{
	stream->Pick(lastModifiedTime);
}

FileSystem::FileSystem()
{
	std::string str = StartupConfig::Get()->executablePath;
	std::replace(str.begin(), str.end(), '\\', '/');

	m_executablePath = FileUtils::PopPath(str.c_str());
	SetCurrentWorkingDirectory(m_executablePath);
}

FileSystem::~FileSystem()
{
	SaveCache();
}

void FileSystem::LoadCache()
{
	ByteStream stream;
	if (ReadCacheStream(".filesystem", &stream))
	{
		auto size = stream.Get<size_t>();
		for (size_t i = 0; i < size; i++)
		{
			auto path = stream.Get<String>();
			FileOrDirectory file = {};
			file.Deserialize(&stream);
			m_indexedFiles.insert({ path, file });
		}
	}
}

void FileSystem::BeginInitializeResourcePaths()
{
	m_isInitializedResourcePaths = false;
}

void FileSystem::EndInitializeResourcePaths()
{
	LoadCache();
	m_isInitializedResourcePaths = true;
}

void FileSystem::SetCurrentWorkingDirectory(const String& path)
{
	//m_executablePath = path;
	m_currentPath = path;
	m_cachePath = path + ".cache/";

	platform::SetCurrentDirectory(path.c_str());
}

void FileSystem::SaveCache()
{
	/*ByteStream stream;
	stream.Put((size_t)3);
	stream.Put(String("Resources/rain1.jpg"));
	stream.Put((size_t)13223309972603);
	stream.Put(String("Resources/buildings/victory_tower_0.png"));
	stream.Put((size_t)13313778521009);
	stream.Put(String("Resources/2.png"));
	stream.Put((size_t)13327853338327);

	WriteStream(".filesystem", &stream);*/

	ByteStream stream;
	stream.Put(m_indexedFiles.size());

	for (auto& [key, value] : m_indexedFiles)
	{
		stream.Put(key);
		value.Serialize(&stream);
	}

	WriteCacheStream(".filesystem", &stream);
}

bool FileSystem::IsFileExisted(const String& relativePath)
{
	if (std::filesystem::is_regular_file(relativePath.c_str()) && std::filesystem::exists(relativePath.c_str()))
	{
		return true;
	}

	if (std::filesystem::path(relativePath.c_str()).is_absolute())
	{
		if (std::filesystem::is_regular_file(relativePath.c_str()) && std::filesystem::exists(relativePath.c_str()))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	//return std::filesystem::is_regular_file(path) && std::filesystem::exists(path);
	return FindRelativeFilePath(relativePath) != "";
}

bool FileSystem::IsDirectoryExisted(const String& relativePath)
{
	if (std::filesystem::is_directory(relativePath.c_str()) && std::filesystem::exists(relativePath.c_str()))
	{
		return true;
	}

	if (std::filesystem::path(relativePath.c_str()).is_absolute())
	{
		if (std::filesystem::is_directory(relativePath.c_str()) && std::filesystem::exists(relativePath.c_str()))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	assert(std::filesystem::path(relativePath.c_str()).is_relative());

	bool ret = false;
	ForEachSearchDirectories(
		[&](const String& wkd, const String& seachDirectory)
		{
			auto p = seachDirectory + relativePath;
			if (std::filesystem::is_directory(p.c_str()) && std::filesystem::exists(p.c_str()))
			{
				ret = true;
				return true;
			}

			return false;
		}
	);

	return ret;
}

size_t FileSystem::GetFileModifiedLastTime(const String& _path)
{
	auto path = FindAbsoluteFilePath(_path);
	assert(fs::is_regular_file(path.c_str()));
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		fs::last_write_time(path.c_str()).time_since_epoch()
	).count();
}

bool FileSystem::IsFileChanged(const String& relativePath, bool updateLastModifiedTime)
{
	auto fullpath = std::filesystem::path(relativePath.c_str()).is_relative() ? FindAbsoluteFilePath(relativePath) : relativePath;

	if (fullpath.empty())
	{
		std::cout << relativePath << "\n";
	}

	assert(!fullpath.empty());

	//std::string_view fullpath = path;
	assert(fs::is_regular_file(fullpath.c_str()));

	auto lastWriteTime = std::chrono::duration_cast<std::chrono::milliseconds>(
		fs::last_write_time(fullpath.c_str()).time_since_epoch()
	).count();

	auto it = m_indexedFiles.find(fullpath);
	if (it != m_indexedFiles.end())
	{
		if (it->second.lastModifiedTime == lastWriteTime)
		{
			return false;
		}

		if (updateLastModifiedTime)
			it->second.lastModifiedTime = lastWriteTime;

		return true;
	}

	FileOrDirectory file = {};
	file.lastModifiedTime = lastWriteTime;
	m_indexedFiles.insert({ fullpath, file });

	return true;
}

bool FileSystem::IsDirectoryChanged(const String& relativePath, bool updateLastModifiedTime)
{
	assert(0);
	return false;
}

void FileSystem::WriteStream(const String& path, ByteStreamRead* stream)
{
	String fullpath = path;//GetCachePath(path);
	
	auto begin = stream->BeginRead();
	auto end = stream->EndRead();
	size_t len = end - begin;

	FILE* fp = fopen(fullpath.c_str(), "wb+");

	if (!fp)
	{
		std::filesystem::path fspath{ fullpath.c_str() };
		std::filesystem::create_directories(fspath.parent_path());
		fp = fopen(fullpath.c_str(), "wb+");
	}

	fwrite(begin, sizeof(byte), len, fp);

	fclose(fp);
}

void FileSystem::WriteCacheStream(const String& path, ByteStreamRead* stream)
{
	WriteStream(GetCachePath(path), stream);
}

bool FileSystem::ReadStream(const String& path, ByteStream* output)
{
	String fullpath = path;//GetCachePath(path);
	if (!IsFileExisted(fullpath.c_str()))
	{
		return false;
	}

	assert(fs::is_regular_file(fullpath.c_str()));

	FILE* fp = fopen(fullpath.c_str(), "rb");
	fseek(fp, 0L, SEEK_END);
	size_t fileSize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	output->Resize(fileSize - output->GetHeaderSize());

	fread(output->BeginRead(), sizeof(byte), fileSize, fp);

	auto offset = fileSize;
	output->CurWrite() = output->BeginRead() + offset;
	output->CurRead() = output->BeginRead() + output->GetHeaderSize();

	fclose(fp);

	return true;
}

bool FileSystem::ReadCacheStream(const String& path, ByteStream* output)
{
	return ReadStream(GetCachePath(path), output);
}

void FileSystem::AddWorkingDirectory(const String& workingDirectory)
{
	assert(std::filesystem::path(workingDirectory.c_str()).is_absolute());
	assert(std::find_if(m_workingDirectories.begin(), m_workingDirectories.end(),
		[&](const WorkingDirectory& wkd)
		{
			return wkd.rootPath == workingDirectory;
		}
	) == m_workingDirectories.end());
	assert(m_isInitializedResourcePaths == false);

	auto& wkd = m_workingDirectories.emplace_back();
	wkd.rootPath = workingDirectory;
}

void FileSystem::AddSearchDirectory(const String& workingDirectory, const String& searchDirectory)
{
	assert(m_isInitializedResourcePaths == false);

	for (auto& wkd : m_workingDirectories)
	{
		if (wkd.rootPath != workingDirectory)
		{
			continue;
		}

		auto path = wkd.rootPath + searchDirectory;
		assert(std::filesystem::is_directory(path.c_str()) && std::filesystem::exists(path.c_str()));

		if (path[path.length() - 1] != '/')
		{
			wkd.searchDirectories.push_back(searchDirectory + "/");
			return;
		}
		wkd.searchDirectories.push_back(searchDirectory);
		return;
	}

	assert(0 && "Working directory is not found.");
}

String FileSystem::FindRelativeFilePath(const String& path, String* outputWkd)
{
	assert(std::filesystem::path(path.c_str()).is_relative());

	if (outputWkd)
	{
		*outputWkd = "";
	}

	if (std::filesystem::exists(path.c_str()) && std::filesystem::is_regular_file(path.c_str()))
	{
		if (outputWkd)
		{
			*outputWkd = GetCurrentWorkingDirectory();
		}
		return path;
	}

	String ret = "";
	ForEachSearchDirectories(
		[&](const String& wkd, const String& seachDirectory)
		{
			auto p = seachDirectory + path;
			if (std::filesystem::exists(p.c_str()) && std::filesystem::is_regular_file(p.c_str()))
			{
				if (outputWkd)
				{
					*outputWkd = wkd;
				}
				ret = p.SubString(wkd.length());
				return true;
			}

			return false;
		}
	);

	return ret;
}

String FileSystem::FindAbsoluteFilePath(const String& path)
{
	if (std::filesystem::path(path.c_str()).is_absolute())
	{
		return path;
	}

	assert(std::filesystem::path(path.c_str()).is_relative());

	String base = "";
	auto relative = FindRelativeFilePath(path, &base);
	return base + relative;
}

String FileSystem::GetRelativeFilePath(const String& absolutePath)
{
	assert(std::filesystem::path(absolutePath.c_str()).is_absolute());
	for (auto& wkd : m_workingDirectories)
	{
		auto idx = absolutePath.Find(wkd.rootPath);
		if (idx == 0)
		{
			return absolutePath.SubString(wkd.rootPath.length());
		}
	}
	assert(0);
	return "";
}

NAMESPACE_END

