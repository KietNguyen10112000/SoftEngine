#pragma once

#include "TypeDef.h"
#include "NewMalloc.h"

NAMESPACE_MEMORY_BEGIN

class StackAllocator
{
private:
	constexpr static size_t ALIGNMENT_SIZE = 16;

	struct Block
	{
		Block* next;
		std::atomic<size_t> allocatedBytes = 0;

		inline byte* FirstByte()
		{
			return (byte*)(this + 1);
		}
	};

	Block* m_reserveBlock = nullptr;
	Block* m_topBlock = nullptr;
	
	size_t m_sizeOfEachBlock = 8 * MB;
	Spinlock m_lock;
	

public:
	StackAllocator(size_t sizeOfEachBlock = 8 * MB) : m_sizeOfEachBlock(sizeOfEachBlock)
	{
		m_topBlock = (Block*)std::malloc(sizeOfEachBlock);
		m_topBlock->allocatedBytes = 0;
		m_topBlock->next = nullptr;
	}

private:
	inline static size_t AlignSize(size_t totalBytes)
	{
		return (totalBytes / ALIGNMENT_SIZE + (totalBytes % ALIGNMENT_SIZE != 0)) * ALIGNMENT_SIZE;
	};

	inline void* GrowthAllocate(size_t nBytes)
	{
		void* ret;
		m_lock.lock();

	Begin:
		auto idx = m_topBlock->allocatedBytes.fetch_add(nBytes);
		if (idx <= m_sizeOfEachBlock)
		{
			ret = m_topBlock->FirstByte() + idx - nBytes;
			goto Return;
		}

		Block* block;
		if (m_reserveBlock != nullptr)
		{
			block = m_reserveBlock;
			m_reserveBlock = m_reserveBlock->next;
		}
		else
		{
			block = (Block*)std::malloc(m_sizeOfEachBlock);
		}

		block->allocatedBytes = 0;
		block->next = m_topBlock;

		m_topBlock = block;

		goto Begin;

	Return:
		m_lock.unlock();
		return ret;
	}

public:
	// thread-safe
	inline void* Allocate(size_t nBytes)
	{
		nBytes = AlignSize(nBytes);

		auto idx = m_topBlock->allocatedBytes.fetch_add(nBytes);
		if (idx <= m_sizeOfEachBlock)
		{
			return m_topBlock->FirstByte() + idx - nBytes;
		}

		return GrowthAllocate(nBytes);
	}

	// no thread-safe
	inline void Clear()
	{
		if (m_reserveBlock == nullptr)
		{
			m_reserveBlock = m_topBlock;
			m_reserveBlock->allocatedBytes = 0;
			m_reserveBlock->next = nullptr;

			m_topBlock = m_topBlock->next;
		}

		auto it = m_topBlock;
		while (it)
		{
			auto next = it->next;

			it->allocatedBytes = 0;
			it->next = m_reserveBlock;
			m_reserveBlock = it;

			it = next;
		}

		m_topBlock = m_reserveBlock;
		m_reserveBlock = m_reserveBlock->next;
	}

};


NAMESPACE_MEMORY_END