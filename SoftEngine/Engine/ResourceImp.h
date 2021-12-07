#pragma once

#include <IResource.h>

#include <Common.h>

#include <map>
#include <string>
#include <File.h>

#include "Global.h"

class Resource
{
private:
	std::map<std::wstring, IResource*> m_map;
	std::mutex m_lock;

	std::wstring m_shaderDir;
	std::wstring m_currentDir;

public:
	Resource();
	~Resource();

private:
	template <typename T>
	T* _Get(const std::wstring& path, uint32_t numArg = 0, void** args = 0);

	template <typename T>
	T* _Get(T* resource);

	template <typename T>
	bool _Release(T** resource);

public:
	template <typename T>
	static T* Get(const std::wstring& path, uint32_t numArg = 0, void** args = 0);

	template <typename T>
	static T* Get(T* resource);

	template <typename T>
	static bool Release(T** resource);

	inline static auto& CurrentDirectory() { return Global::resourceManager->m_currentDir; };
	inline static auto& ShaderDirectory() { return Global::resourceManager->m_shaderDir; };

};

inline Resource::Resource()
{
	wchar_t buffer[_MAX_DIR] = {};
	GetCurrentDirectoryW(_MAX_DIR, buffer);
	m_currentDir = buffer;

	StandardPath(m_currentDir);

	m_shaderDir = m_currentDir;
}

inline Resource::~Resource()
{
}


#define __GET_KEY(path) \
std::wstring key = path + L'\0' + StringToWString(typeid(T).name());

template<typename T>
inline T* Resource::_Get(const std::wstring& path, uint32_t numArg, void** args)
{
	//std::wstring key = path + StringToWString(typeid(T)._Data._DecoratedName);
	__GET_KEY(path);

	WaitToLock(m_lock);
	auto it = m_map.find(key);

	if (it == m_map.end())
	{
		::Release(m_lock);
		T* re = new T(path, numArg, args);

		WaitToLock(m_lock);
		it = m_map.find(key);

		if (it != m_map.end())
		{
			delete re;
			it->second->m_refCount++;
			re = dynamic_cast<T*>(it->second);
			::Release(m_lock);
			return re;
		}
		else
		{
			auto ret = dynamic_cast<IResource*>(re);
			ret->m_refCount = 1;
			m_map.insert({ { key } , ret });
			::Release(m_lock);
			return re;
		}
	}
	else
	{
		it->second->m_refCount++;
		T* re = dynamic_cast<T*>(it->second);
		::Release(m_lock);
		return re;
	}
}

template<typename T>
inline T* Resource::_Get(T* resource)
{
	WaitToLock(m_lock);
	(dynamic_cast<IResource*>(resource))->m_refCount++;
	::Release(m_lock);
	return resource;
}

template<typename T>
inline bool Resource::_Release(T** resource)
{
	if (!(*resource)) return 0;

	auto target = dynamic_cast<IResource*>(*resource);

	WaitToLock(m_lock);
	target->m_refCount--;

	if (target->m_refCount > 0)
	{
		*resource = nullptr;
		::Release(m_lock);
		return 1;
	}

	auto& path = (*resource)->Path();
	__GET_KEY(path);
	auto it = m_map.find(key);

	if (it == m_map.end())
	{
		::Release(m_lock);
		return false;
	}
	else
	{
		m_map.erase(it);
		::Release(m_lock);
		delete* resource;
		*resource = nullptr;
		return true;
	}
}

template<typename T>
inline T* Resource::Get(const std::wstring& path, uint32_t numArg, void** args)
{
	return Global::resourceManager->_Get<T>(path, numArg, args);
}

template<typename T>
inline T* Resource::Get(T* resource)
{
	//(dynamic_cast<IResource*>(resource))->m_refCount++;
	return Global::resourceManager->_Get<T>(resource);
}

template<typename T>
inline bool Resource::Release(T** resource)
{
	return Global::resourceManager->_Release<T>(resource);
}
