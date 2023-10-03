#include "FileSystem.h"

#include "Core/Memory/NewMalloc.h"

#include "Runtime/StartupConfig.h"

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
	std::string str = StartupConfig::Get().executablePath;
	std::replace(str.begin(), str.end(), '\\', '/');
	auto path = fs::path(str);
	
	m_cachePath = String(path.parent_path().u8string().c_str()) + "/.cache/";
	m_rootPath = StartupConfig::Get().resourcesPath;

	LoadCache();
}

FileSystem::~FileSystem()
{
	SaveCache();
}

void FileSystem::LoadCache()
{
	ByteStream _stream;
	if (ReadStream(".filesystem", &_stream))
	{
		auto stream = ByteStreamRead::From(_stream);
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

void FileSystem::SaveCache()
{
	ByteStream stream;
	stream.Put(m_indexedFiles.size());

	for (auto& [key, value] : m_indexedFiles)
	{
		stream.Put(key);
		value.Serialize(&stream);
	}

	WriteStream(".filesystem", &stream);
}

bool FileSystem::IsFileChanged(const char* path, bool updateLastModifiedTime)
{
	auto fullpath = GetFullPath(path);
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

bool FileSystem::IsDirectoryChanged(const char* path, bool updateLastModifiedTime)
{
	assert(0);
	return false;
}

void FileSystem::WriteStream(const char* path, ByteStreamRead* stream)
{
	auto fullpath = GetCachePath(path);
	
	auto begin = stream->begin();
	auto cur = stream->cur();
	size_t len = cur - begin;

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

bool FileSystem::ReadStream(const char* path, ByteStream* output)
{
	auto fullpath = GetCachePath(path);
	if (!IsFileExist(fullpath.c_str()))
	{
		return false;
	}

	assert(fs::is_regular_file(fullpath.c_str()));

	FILE* fp = fopen(fullpath.c_str(), "rb");
	fseek(fp, 0L, SEEK_END);
	size_t fileSize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	output->Resize(fileSize - output->GetHeaderSize());

	fread(output->begin(), sizeof(byte), fileSize, fp);

	auto offset = fileSize;
	output->m_cur = output->begin() + offset;

	fclose(fp);

	return true;
}

NAMESPACE_END

