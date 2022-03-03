#pragma once

#include <iostream>

class RefCounted
{
public:
	int m_refCount = 0;

};

template <typename T, bool AUTO_DELETE = true>
class Ref
{
public:
	static_assert(std::is_base_of_v<RefCounted, T>);
	T* m_data = 0;

public:
	inline Ref() {};

	inline Ref(T* left)
	{
		m_data = left;
		m_data->m_refCount++;
	};

	inline ~Ref()
	{
		m_data->m_refCount--;

		if constexpr (AUTO_DELETE)
		{
			if (m_data->m_refCount == 0)
				delete m_data;
		}
	};

public:
	inline void operator=(T* left) const noexcept
	{
		m_data = left;
		m_data->m_refCount++;
	};

	inline T& operator*() const noexcept
	{
		return *m_data;
	};

	inline T* operator->() const noexcept
	{
		return m_data;
	};

	inline operator T* () const noexcept 
	{ 
		return m_data; 
	};

};