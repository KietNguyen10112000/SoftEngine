#pragma once

#include "TypeDef.h"

NAMESPACE_MEMORY_BEGIN


// just alloc, delete when program exits =)))
class SimpleStackBasedAllocator
{
public:
	struct Stack
	{
		Stack* next;
		byte* current;
		byte* top;

		inline size_t Size()
		{
			return top - current;
		}

		inline size_t TotalAllocatedBytes()
		{
			return current - (byte*)(this + 1);
		}

		inline byte* Allocate(size_t nBytes)
		{
			auto ret = current;
			current += nBytes;
			assert(current <= top);
			return ret;
		}
	};

	const size_t m_K = 1;
	size_t m_stackSize = 0;
	Stack* m_head = 0;

public:
	inline SimpleStackBasedAllocator(size_t stackSize, size_t K = 1) : m_K(K)
	{
		m_stackSize = stackSize;
		m_head = (Stack*)malloc(m_stackSize + sizeof(Stack));
		InitStack(m_head);
	}

	inline ~SimpleStackBasedAllocator()
	{
		auto it = m_head;
		while (it)
		{
			auto next = it->next;
			free(it);
			it = next;
		}
	}

protected:
	inline void InitStack(Stack* s)
	{
		s->next = 0;
		s->current = (byte*)(s + 1);
		s->top = s->current + m_stackSize;
	}

	inline void AllocateNextStack(Stack* s)
	{
		m_stackSize *= m_K;
		auto next = (Stack*)malloc(m_stackSize + sizeof(Stack));
		s->next = next;
		InitStack(next);
	}

public:
	// no align
	inline byte* Allocate(size_t size)
	{
		byte* ret = 0;

		auto it = m_head;
		auto prev = it;
		while (it)
		{
			if (it->Size() >= size)
			{
				ret = it->Allocate(size);
				break;
			}
			prev = it;
			it = it->next;
		}

		if (ret == 0)
		{
			AllocateNextStack(prev);
			it = prev->next;
			ret = it->Allocate(size);
			assert(ret != 0);
		}

		return ret;
	}

	// no deallocate =)))


	inline size_t TotalAllocatedBytes()
	{
		size_t size = 0;
		auto it = m_head;
		while (it)
		{
			size += it->TotalAllocatedBytes();
			it = it->next;
		}
		return size;
	}

};


NAMESPACE_MEMORY_END