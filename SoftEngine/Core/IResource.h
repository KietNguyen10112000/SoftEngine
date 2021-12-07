#pragma once
#include <string>

class IResource
{
protected:
	std::wstring m_path;
	uint32_t m_refCount = 0;

	friend class Resource;

public:
	inline IResource(const std::wstring& path, uint32_t numArg = 0, void** args = 0) : m_path(path) {};
	inline virtual ~IResource() {};

public:
	virtual const std::wstring& Path() const { return m_path; };

};