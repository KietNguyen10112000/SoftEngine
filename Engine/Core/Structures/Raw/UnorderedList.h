#pragma once
#include "Core/TypeDef.h"

#include "Core/Memory/Memory.h"
#include "Core/Memory/Pool.h"

#include "../STD/STDContainers.h"


NAMESPACE_BEGIN

namespace raw
{

template<typename T>
class UnorderedVector
{
private:
	std::Vector<T> m_list = {};
	std::Vector<size_t> m_freeIds = {};
	size_t m_size = 0;


};

template<typename T, size_t GROWTH_SIZE = 128>
class UnorderedLinkedList
{
public:
	struct Node
	{
		Node* next;
		Node* prev;
		T value;
	};

private:
	Pool<sizeof(Node), GROWTH_SIZE, 1, rheap::malloc, rheap::free> m_mem;

	Node* m_head = nullptr;

public:
	inline ID Add(const T& v)
	{
		Node* node = (Node*)m_mem.Allocate();

		node->next = m_head;

		if (m_head)
		{
			m_head->prev = node;
		}

		m_head = node;

		return (ID)node;
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
	}
};

template<typename T, size_t GROWTH_SIZE = 128>
using UnorderedList = UnorderedLinkedList<T, GROWTH_SIZE>;

}

NAMESPACE_END