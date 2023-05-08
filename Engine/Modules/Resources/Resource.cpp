#include "Resource.h"

NAMESPACE_BEGIN

namespace resource
{

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
	new (&v) RCMap();
}

template <typename T>
inline void FreeGcMap(T& gcMap)
{
	gcMap.~T();
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

void Initialize()
{
	InitRcMap(GetRcMap());
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

}

NAMESPACE_END