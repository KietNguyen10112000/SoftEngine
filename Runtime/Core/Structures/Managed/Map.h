#pragma once
#include "Core/Memory/ManagedPointers.h"
#include "Core/Memory/Trace.h"
#include "Core/Structures/TypeDef.h"
#include "Core/Memory/Memory.h"

#include "Array.h"

#include <map>

NAMESPACE_BEGIN

template <typename K, typename V>
class Map;

struct _MapUtilStruct
{
	template <typename K, typename V>
	struct IteratorNormal
	{
		K key;
		V value;

	protected:
		friend class Map<K, V>;

		using It = typename std::map<K, ID>::iterator;

		It it;
		Map<K, V>* map;

	public:
		inline bool operator==(const IteratorNormal<K, V>& _it) const
		{
			return it == _it.it && map == _it.map;
		}

		inline void operator++()
		{
			it++;
			if (it == map->end())
			{
				key = {};
				value = {};
			}
			else
			{
				key = it->first;
				value = map->m_array[it->second];
			}
		}

	};

	template <typename K, typename V>
	struct IteratorTraceable : public IteratorNormal<K, V>
	{
	private:
		friend class Map<K, V>;
		TRACEABLE_FRIEND();
		inline void Trace(Tracer* tracer)
		{
			tracer->Trace(this->value);
		}
	};
};

template <typename K, typename V>
class Map
{
private:
	Array<V> m_array;
	std::map<K, ID> m_ids;

	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_array);
	}

public:
	using Iterator = typename std::conditional<
		Tracer::IsTraceable<V>(),
		typename _MapUtilStruct::IteratorTraceable<K, V>,
		typename _MapUtilStruct::IteratorNormal<K, V>
	>::type;

	friend typename Iterator;

public:
	inline V& operator[](const K& key)
	{
		auto it = m_ids.find(key);
		if (it == m_ids.end())
		{
			auto id = m_array.size();
			m_array.Push({});
			m_ids.insert({ key,id });
			return m_array[id];
		}

		return m_array[it->second];
	}

	inline Iterator insert(const K& key, const V& value)
	{
		auto id = m_array.size();
		m_array.Push(value);
		auto it = m_ids.insert({ key,id });

		Iterator ret;
		ret.key = key;
		ret.value = value;
		ret.it = it;
		ret.map = this;

		return ret;
	}

	inline Iterator find(const K& key, const V& value)
	{
		Iterator ret;
		ret.it = m_ids.find(key);
		ret.map = this;
		if (ret.it == m_ids.end())
		{
			return ret;
		}

		ret.key = ret.it->first;
		ret.value = m_array[ret.it->second];

		return ret;
	}

	inline Iterator begin()
	{
		Iterator ret;
		ret.it = m_ids.begin();
		ret.map = this;
		if (ret.it == m_ids.end())
		{
			return ret;
		}

		ret.key = ret.it->first;
		ret.value = m_array[ret.it->second];
		return ret;
	}

	inline Iterator end()
	{
		Iterator ret;
		ret.it = m_ids.end();
		ret.map = this;
		return ret;
	}
};

NAMESPACE_END