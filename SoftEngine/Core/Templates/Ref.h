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
		if (m_data)
			m_data->m_refCount--;

		m_data = left;
		m_data->m_refCount++;
	};

	inline Ref(const Ref<T>& left)
	{
		if (m_data)
			m_data->m_refCount--;

		m_data = left.m_data;
		m_data->m_refCount++;
	};

	inline ~Ref()
	{
		if (m_data)
			m_data->m_refCount--;

		if constexpr (AUTO_DELETE)
		{
			if (m_data->m_refCount == 0)
				delete m_data;
		}
	};

public:
	inline void operator=(Ref<T>& left) noexcept
	{
		if (m_data)
			m_data->m_refCount--;

		m_data = left.m_data;
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

	inline Ref<T>& operator=(Ref<T>&& left) noexcept
	{
		if (m_data)
			m_data->m_refCount--;

		m_data = left;
		m_data->m_refCount++;

		return *this;
	};
//private:
//	inline void operator=(Ref<T> left) const noexcept
//	{
//		
//	};

};