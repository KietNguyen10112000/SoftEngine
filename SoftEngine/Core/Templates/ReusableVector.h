#pragma once

#include <vector>

template <typename T>
class ReusableVector
{
public:
	std::vector<T> m_mem;
	std::vector<size_t> m_freeSlots;

public:
	inline size_t NextIndex()
	{
		size_t ret = 0;

		if (m_freeSlots.empty())
		{
			ret = m_mem.size();
			m_mem.resize(ret + 1);
		}
		else
		{
			ret = m_freeSlots.back();
			m_freeSlots.pop_back();
		}

		return ret;
	};

	inline void Return(size_t index)
	{
		m_freeSlots.push_back(index);
	};

	inline void Clear()
	{
		m_mem.clear();
		m_freeSlots.clear();
	};

	inline T& operator[](size_t index)
	{
		return m_mem[index];
	};

};