#pragma once

#include "Array.h"

#include "../STD/STDContainers.h"

NAMESPACE_BEGIN

template<typename T, size_t GROWTH_SIZE = 128>
class UnorderedList : Traceable<UnorderedList<T, GROWTH_SIZE>>
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
			callback(v, (ID)m_signs[id++]);
		}
	}

};

template<typename T>
class UnorderedLinkedList : public Traceable<UnorderedLinkedList<T>>
{
private:
	struct Node : public Traceable<Node>
	{
		Handle<Node> next;
		Handle<Node> prev;
		T value;

		TRACEABLE_FRIEND();
		void Trace(Tracer* tracer)
		{
			tracer->Trace(next);
			tracer->Trace(prev);

			if constexpr (std::is_base_of_v<Traceable<T>, T>)
			{
				tracer->Trace(value);
			}
		}
	};

	Handle<Node> m_head = nullptr;
	size_t m_size = 0;

	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_head);
	}

public:
	inline ID Add(const T& v)
	{
		Handle<Node> node = mheap::New<Node>();

		node->prev = nullptr;
		node->next = m_head;

		if (m_head)
		{
			m_head->prev = node;
		}

		m_head = node;

		m_size++;

		node->value = v;

		return (ID)node.Get();
	}

	inline void Remove(ID id)
	{
		Node* node = (Node*)id;

		auto prev = node->prev;
		auto next = node->next;

		if (prev)
		{
			prev->next = next;
		}
		else
		{
			m_head = next;
		}

		if (next)
		{
			next->prev = prev;
		}

		m_size--;
	}

	inline auto size() const
	{
		return m_size;
	}


	inline ID backId() const
	{
		return (ID)m_head.Get();
	}

	inline T& back() const
	{
		return m_head->value;
	}

	inline const T& Get(ID id) const
	{
		Node* node = (Node*)id;
		return node->value;
	}

	inline void Clear()
	{
		m_head = nullptr;
		m_size = 0;
	}

public:
	template <typename F>
	inline void ForEach(F callback)
	{
		auto it = m_head.Get();
		while (it)
		{
			callback(it->value);
			it = it->next.Get();
		}
	}

};

NAMESPACE_END