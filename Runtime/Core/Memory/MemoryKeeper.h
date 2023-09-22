#pragma once

#include "TypeDef.h"

#include "Pool.h"
#include "MemoryUtils.h"

#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_MEMORY_BEGIN

// not own memory but allocate and deallocate that memory, it use the same technical of Page - BST-based memory allocation
class MemoryKeeper
{
public:
	struct Block
	{
		size_t allocated = 0;
		byte* mem = nullptr;
		size_t size = 0;

		// same size
		Block* sameSizePrev = nullptr;
		Block* sameSizeNext = nullptr;

		Block* prev = nullptr;
		Block* next = nullptr;
	};

private:
	static bool Comparator(Block* b1, Block* b2)
	{
		return b1->size < b2->size;
	}

	Pool<sizeof(Block), sizeof(Block) * 1024> m_poolBlock = { 1 };

	byte* m_begin = nullptr;
	byte* m_end = nullptr;

	std::set<Block*, decltype(Comparator)*, STLNodeGlobalAllocator<Block*>> m_freeBlock;

public:
	MemoryKeeper() : m_freeBlock(Comparator)
	{

	}

	inline void Reset(void* buffer, size_t size)
	{
		m_freeBlock.clear();

		m_begin = (byte*)buffer;
		m_end = (byte*)buffer + size;

		auto block = NewBlock();
		block->mem = m_begin;
		block->size = size;
		block->prev = 0;
		block->next = 0;

		m_freeBlock.insert(block);
	}

private:
	inline Block* NewBlock()
	{
		auto block = (Block*)m_poolBlock.Allocate();
		new (block) Block();
		return block;
	}

	inline void DeleteBlock(Block* block)
	{
		m_poolBlock.Deallocate(block);
	}

	inline Block* BestFit(size_t nBytes)
	{
		Block fake;
		fake.size = nBytes;

		auto it = m_freeBlock.lower_bound(&fake);
		if (it != m_freeBlock.end())
		{
			auto ret = *it;

			if (!ret->sameSizeNext)
				m_freeBlock.erase(it);

			return ret;
		}

		return nullptr;
	}

	inline void RemoveFreeBlock(Block* block)
	{
		if (block->sameSizeNext)
		{
			block->sameSizeNext->sameSizePrev = block->sameSizePrev;
		}

		if (block->sameSizePrev)
		{
			block->sameSizePrev->sameSizeNext = block->sameSizeNext;
		}
		else
		{
			m_freeBlock.erase(block);
			if (block->sameSizeNext)
			{
				m_freeBlock.insert(block->sameSizeNext);
				block->sameSizeNext->sameSizePrev = nullptr;
			}
		}
	}

	inline void InsertFreeBlock(Block* _block)
	{
		auto it = m_freeBlock.find(_block);
		if (it != m_freeBlock.end())
		{
			auto block = *it;
			assert(block->sameSizePrev == nullptr);

			_block->sameSizeNext = block->sameSizeNext;
			if (_block->sameSizeNext)
				_block->sameSizeNext->sameSizePrev = _block;

			_block->sameSizePrev = block;
			block->sameSizeNext = _block;
		}
		else
		{
			m_freeBlock.insert(_block);
		}
	}

public:
	Block* Allocate(size_t nBytes)
	{
		nBytes = MemoryUtils::Align<16>(nBytes);

		auto bestFit = BestFit(nBytes);
		if (!bestFit)
		{
			return nullptr;
		}

		Block* choseBlock = bestFit;
		if (bestFit->sameSizeNext)
		{
			auto next = bestFit->sameSizeNext;
			bestFit->sameSizeNext = next->sameSizeNext;

			if (bestFit->sameSizeNext)
				bestFit->sameSizeNext->sameSizePrev = bestFit;

			choseBlock = next;
		};

		if ((int64_t)choseBlock->size - (int64_t)nBytes < 16)
		{
			choseBlock->allocated = true;
			return choseBlock;
		}

		Block* ret = NewBlock();
		ret->mem = choseBlock->mem;
		ret->size = nBytes;
		ret->next = choseBlock;
		ret->prev = choseBlock->prev;
		ret->allocated = true;

		// split choseBlock
		{
			choseBlock->sameSizeNext = nullptr;
			choseBlock->sameSizePrev = nullptr;
			choseBlock->mem += nBytes;
			choseBlock->size -= nBytes;

			if (choseBlock->prev)
				choseBlock->prev->next = ret;

			choseBlock->prev = ret;

			InsertFreeBlock(choseBlock);
		}

		return ret;
	}

	void Deallocate(Block* block)
	{
		auto prev = block->prev;
		auto next = block->next;

		if (prev && !prev->allocated)
		{
			RemoveFreeBlock(prev);

			assert(prev->mem + prev->size == block->mem);

			block->mem = prev->mem;
			block->size += prev->size;
			block->prev = prev->prev;

			if (block->prev)
				block->prev->next = block;

			if (!prev->sameSizePrev)
				DeleteBlock(prev);
		}

		if (next && !next->allocated)
		{
			RemoveFreeBlock(next);

			assert(block->mem + block->size == next->mem || block->mem + block->size == m_end);

			block->size += next->size;
			block->next = next->next;

			if (block->next)
				block->next->prev = block;

			if (!next->sameSizePrev)
				DeleteBlock(next);
		}

		block->sameSizePrev = nullptr;
		block->sameSizeNext = nullptr;
		block->allocated = false;
		InsertFreeBlock(block);
	}

};

NAMESPACE_MEMORY_END