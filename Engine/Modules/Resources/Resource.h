#pragma once

#include "Core/Memory/Memory.h"

#include "Core/Structures/String.h"

#include "FileSystem/FileUtils.h"

#include "Engine/StartupConfig.h"

NAMESPACE_BEGIN

template <typename T>
class Resource;

class ResourceBase
{
private:
	template <typename T>
	friend class Resource;

	friend class ResourceBaseAccessor;

	std::atomic<size_t> m_refCount = { 0 };
	String m_path;
	String m_key;

public:
	ResourceBase(String path) : m_path(path) {};
	virtual ~ResourceBase() {};

public:
	inline auto GetPath() const
	{
		return m_path;
	}

};

class ResourceBaseAccessor
{
public:
	inline static void SetKey(ResourceBase* rc, String key)
	{
		rc->m_key = key;
	}

	inline static String GetKey(ResourceBase* rc)
	{
		return rc->m_key;
	}

};

namespace resource
{
	namespace internal
	{
		using ResourceBaseClass = ::soft::ResourceBase;
		API ResourceBaseClass* TryLoad(String path, const char* resourceClassName);
		API void Assign(ResourceBaseClass* rc, const char* resourceClassName);
		API void Release(ResourceBaseClass* rc);

		void Initialize();
		void Finalize();
	}

	template <typename _T, typename... Args>
	Resource<_T> Load(String path, Args&&... args);
}


template <typename T>
class Resource
{
private:
	static_assert(std::is_base_of_v<ResourceBase, T>);

	 template <typename _T, typename... Args>
	 friend Resource<_T> resource::Load(String path, Args&&... args);


	T* m_rc = nullptr;

	Resource(T* rc) : m_rc(rc)
	{
		rc->m_refCount++;
	}

public:
	Resource() {};

	template <typename _T>
	Resource(const Resource<_T>& rc)
	{
		*this = rc;
	}

	~Resource()
	{
		Reset();
	}

	inline void Reset()
	{
		if (m_rc && (--(m_rc->m_refCount)) == 0)
		{
			resource::internal::Release(m_rc);
			m_rc = 0;
		}
	}

	template <typename _T>
	inline void operator=(const Resource<_T>& rc)
	{
		Reset();
		m_rc = rc.m_rc;
		m_rc->m_refCount++;
	}

	inline void operator=(const Resource<T>& rc)
	{
		Reset();
		m_rc = rc.m_rc;
		m_rc->m_refCount++;
	}

	inline T* operator->() const
	{
		return m_rc;
	}

	inline operator T* () const
	{
		return m_rc;
	}

};

namespace resource
{

template <typename _T, typename... Args>
inline Resource<_T> Load(String path, Args&&... args)
{
	_T* rc = dynamic_cast<_T*>(internal::TryLoad(path, typeid(_T).name()));

	if (!rc)
	{
		rc = rheap::New<_T>(path, std::forward<Args>(args)...);
		internal::Assign(rc, typeid(_T).name());
	}
	
	return Resource<_T>(rc);
}


// callback = function(byte* buffer, size_t size)
// relativePath is relative path to StartupConfig.resourcePath
// return true if success
template <typename Fn>
inline bool ReadFile(String relativePath, Fn callback)
{
	byte* buffer; size_t size;
	FileUtils::ReadFile(StartupConfig::Get().resourcesPath + relativePath, buffer, size);
	callback(buffer, size);
	FileUtils::FreeBuffer(buffer);
	return true;
}

}

NAMESPACE_END