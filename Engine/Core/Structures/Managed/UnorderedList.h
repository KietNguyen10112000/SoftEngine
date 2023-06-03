#pragma once

#include "Array.h"

#include "../STD/STDContainers.h"

NAMESPACE_BEGIN

template<typename T, size_t GROWTH_SIZE = 128>
class UnorderedList : Traceable<UnorderedList<T>>
{
private:
	struct Sign
	{
		size_t index;
	};

	Pool0<GROWTH_SIZE * 4, 1, rheap::malloc, rheap::free> m_signsMem = { 1 };

	Array<T> m_elms;
	std::Vector<Sign*> m_signs = {};

private:
	friend class Tracer;
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_elms);
	}

public:
	//inline UnorderedList() {};

	inline ID Add(const T& v)
	{
		Sign* ret = (Sign*)m_signsMem.Allocate();
		ret->index = m_elms.Size();

		m_signs.push_back(ret);
		m_elms.Push(v);

		return (ID)ret;
	}

	inline void Remove(ID id)
	{
		// use back position to fill inline blank position
		Sign* sign = (Sign*)id;
		
		auto index = sign->index;

		// validate id, id must belong to this list
		assert(sign == m_signs[index]);

		auto& back = m_elms.back();
		auto& backSign = m_signs.back();

		auto& elm = m_elms[index];
		auto& elmSign = m_signs[index];

		if (&back != &elm)
		{
			backSign->index = index;
			
			elm = back;
			elmSign = backSign;
		}
		
		m_elms.Pop();
		m_signs.pop_back();
	}

	inline void Clear()
	{
		m_elms.clear();
		m_signs.clear();
	}

	inline auto& Get(ID id)
	{
		return m_elms[((Sign*)id)->index];
	}

	template<typename Func>
	inline void ForEach(Func callback)
	{
		for (auto& v : m_elms)
		{
			callback(v);
		}
	}

	template<typename Func>
	inline void ForEachWithID(Func callback)
	{
		size_t id = 0;
		for (auto& v : m_elms)
		{
			callback(v, id++);
		}
	}

};

NAMESPACE_END