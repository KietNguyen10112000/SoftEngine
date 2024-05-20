#include "Resource.h"

#include "Core/Pattern/Singleton.h"

#include "Common/Stream/ByteStream.h"

#include "FileSystem/FileSystem.h"

NAMESPACE_BEGIN

namespace resource
{

class ResourceManager : public Singleton<ResourceManager>
{
public:
	inline static const char* META_PATH = "Meta/.resources";

	std::map<String, UUID> m_uuidMap;

	spinlock m_lock;

	ResourceManager()
	{
		ByteStream stream;
		if (FileSystem::Get()->ReadStream(META_PATH, &stream))
		{
			auto size = stream.Get<size_t>();
			for (size_t i = 0; i < size; i++)
			{
				auto uuid = stream.Get<UUID>();
				auto path = stream.Get<String>();
				m_uuidMap.insert({ path,uuid });
			}
		}
	}

	~ResourceManager()
	{
		ByteStream stream;
		stream.Put(m_uuidMap.size());
		for (auto& [key, value] : m_uuidMap)
		{
			stream.Put(value);
			stream.Put(key);
		}

		FileSystem::Get()->WriteStream(META_PATH, &stream);
	}

	inline UUID GetResourceUUID(const String& path)
	{
		m_lock.lock();

		auto it = m_uuidMap.find(path);
		if (it != m_uuidMap.end())
		{
			m_lock.unlock();
			return it->second;
		}

		auto uuid = UUIDGenerator::Get()->GetUUID();
		m_uuidMap.insert({ path,uuid });
		m_lock.unlock();
		return uuid;
	}

};

namespace internal
{

using RCMap = std::map<String, ResourceBaseClass*>;

// key = className[path]
byte g_rcMap[sizeof(RCMap)];
spinlock g_rcMapLock;

inline auto& GetRcMap()
{
	return reinterpret_cast<RCMap&>(g_rcMap);
}

template <typename T>
inline void InitRcMap(T& v)
{
	ResourceManager::SingletonInitialize();

	new (&v) RCMap();
}

template <typename T>
inline void FreeGcMap(T& gcMap)
{
	gcMap.~T();

	ResourceManager::SingletonFinalize();
}

std::map<String, ResourceBaseClass*>* GetInternalRCMap()
{
	auto& map = GetRcMap();
	return &GetRcMap();
}

ResourceBaseClass* TryLoad(String path, const char* resourceClassName)
{
	ResourceBaseClass* ret = nullptr;
	g_rcMapLock.lock();

	auto it = GetRcMap().find(String::Format("{}[{}]", resourceClassName, path));
	if (it != GetRcMap().end())
	{
		ret = it->second;
	}

	g_rcMapLock.unlock();
	return ret;
}

void Assign(ResourceBaseClass* rc, const char* resourceClassName)
{
	auto key = String::Format("{}[{}]", resourceClassName, rc->GetPath());

	g_rcMapLock.lock();
	GetRcMap().insert({ key, rc });
	g_rcMapLock.unlock();

	ResourceBaseAccessor::SetKey(rc, key);
}

void Release(ResourceBaseClass* rc)
{
	auto key = ResourceBaseAccessor::GetKey(rc);
	g_rcMapLock.lock();
	GetRcMap().erase(key);
	g_rcMapLock.unlock();
	rheap::Delete(rc);
}

UUID resource::internal::GetResourceUUID(const String& path)
{
	return ResourceManager::Get()->GetResourceUUID(path);
}

void Finalize()
{
	auto& rcMap = GetRcMap();
	for (auto& [key, value] : GetRcMap())
	{
		//rheap::Delete(value);
		std::cout << "Resource leaks: " << key << "\n";
	}
	GetRcMap().clear();
	FreeGcMap(GetRcMap());
}

}

void SerializeToJson(Serializer* serializer, json& j)
{
	internal::g_rcMapLock.lock();

	auto& map = internal::GetRcMap();
	for (auto& [key, value] : map)
	{
		value->GetPath();
	}

	internal::g_rcMapLock.unlock();
}

void DeserializeFromJson(Serializer* serializer, const json& j)
{
}

void SerializeToBinary(Serializer* serializer, ByteStream& stream)
{

}

void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{

}

void internal::Initialize()
{
	InitRcMap(GetRcMap());

}

}

NAMESPACE_END