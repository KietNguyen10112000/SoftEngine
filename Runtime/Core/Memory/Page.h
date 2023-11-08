#pragma once
#include "AVLTree.h"
#include "TypeDef.h"

#include <cassert>

NAMESPACE_MEMORY_BEGIN

class FreeBlock
{
public:
	bf_t m_bf = 0;
	FreeBlock* m_parent = 0;
	FreeBlock* m_left = 0;
	FreeBlock* m_right = 0;

	// can calculate next block on dense buffer from here
	size_t m_totalSize = 0;

	// linked-list of same size blocks, on avl tree
	FreeBlock* m_next = 0;

	// prev block on dense buffer
	union
	{
		FreeBlock* m_prev = 0;
		AllocatedBlock* m_prev_;
	};

	//size_t m_mem;


public:
	inline auto& BF() { return m_bf; };
	inline auto& Parent() { return m_parent; };
	inline auto& Left() { return m_left; };
	inline auto& Right() { return m_right; };
	inline auto& Key() { return m_totalSize; };

	inline void Print()
	{
		std::cout << "[bf: " << m_bf << ", size: " << m_totalSize << "]\n";
	};

	inline auto TotalSize() { return m_totalSize; };

	inline FreeBlock* NextBlock()
	{
		return (FreeBlock*)((byte*)this + m_totalSize);
	};

	inline FreeBlock* PrevBlock()
	{
		return m_prev;
	};

	inline bool IsAllocated()
	{
		return m_bf > 2 || m_bf < -2;
	};
};

class Page : public avl::Tree<FreeBlock>
{
public:
	constexpr static size_t DEFAULT_SIZE = 16 * 1024 * 1024;

#ifdef ENV64
	constexpr static size_t ALIGNMENT_SIZE = 16;
#else
	constexpr static size_t ALIGNMENT_SIZE = 8;
#endif // ENV64

	using Base = avl::Tree<FreeBlock>;
	using Tree = avl::Tree<FreeBlock>;

public:
	byte* m_buffer = 0;
	size_t m_size = 0;

	Tree::node* m_maxSizeFreeBlock = 0;
	size_t m_totalFreeBlocks = 0;

	size_t m_totalAllocatedBytes = 0;

public:
	Page(bool placeholder)
	{

	}

	Page(size_t size = DEFAULT_SIZE)
	{
		m_buffer = (byte*)malloc(size);
		m_size = size;

		m_maxSizeFreeBlock = (Tree::node*)m_buffer;
		m_totalFreeBlocks = 1;
		Tree::Insert(InitFreeBlock(m_buffer, m_size, 0));
	}

	~Page()
	{
		if (m_buffer) free(m_buffer);
		m_buffer = 0;
	}

protected:
	inline void Initialize(size_t size)
	{
		m_buffer = 0;
		m_size = 0;
		m_maxSizeFreeBlock = 0;
		m_totalFreeBlocks = 0;
		m_totalAllocatedBytes = 0;

		m_buffer = (byte*)malloc(size);
		m_size = size;

		m_maxSizeFreeBlock = (Tree::node*)m_buffer;
		m_totalFreeBlocks = 1;
		Tree::Insert(InitFreeBlock(m_buffer, m_size, 0));
	}

public:
	FreeBlock* InitFreeBlock(byte* addr, size_t totalSize, byte* prev)
	{
		FreeBlock* block = (FreeBlock*)addr;
		block->BF() = 0;
		block->Parent() = 0;
		block->Left() = 0;
		block->Right() = 0;
		block->Key() = totalSize;
		block->m_next = 0;
		block->m_prev = (FreeBlock*)prev;
		return block;
	}

public:
	inline static size_t AlignSize(size_t totalBytes)
	{
		return (totalBytes / ALIGNMENT_SIZE + (totalBytes % ALIGNMENT_SIZE != 0)) * ALIGNMENT_SIZE;
	};

	FreeBlock* BestFit(size_t totalSize)
	{
		auto cur = find_Insert(totalSize);

		FreeBlock* best = 0;

		if (cur && cur->TotalSize() >= totalSize)
		{
			best = cur;
			goto end;
		}

		// retrace
		for (auto it = cur->Parent(); it != nullptr; it = it->Parent())
		{
			if (it->TotalSize() >= totalSize)
			{
				best = it;
				goto end;
			}
		}

	end:
		return best;
	}

	inline bool SplitMemory(size_t size, FreeBlock* memory, FreeBlock*& out)
	{
		assert(AlignSize(size) == size);
		//int64_t numChunk = size / ALIGNMENT_SIZE + (size % ALIGNMENT_SIZE != 0);
		//auto numMemoyChunk = memory->TotalSize() / ALIGNMENT_SIZE;
		//auto remainChunk = numMemoyChunk - numChunk;
		//auto remainBytes = remainChunk * ALIGNMENT_SIZE;

		auto remainBytes = memory->TotalSize() - size;

		if (remainBytes >= sizeof(FreeBlock))
		{
			//out = InitFreeBlock((byte*)memory + (numChunk * ALIGNMENT_SIZE), remainChunk * ALIGNMENT_SIZE, (byte*)memory);
			out = InitFreeBlock((byte*)memory + size, remainBytes, (byte*)memory);
			return true;
		}

		return false;
	}

	// fill result to ptr and totalSize
	inline void ReturnAndJoin(AllocatedBlock* returnBlock, byte*& ptr, size_t& totalSize,
		FreeBlock*& deletedPrev, FreeBlock*& deletedNext)
	{
		ptr = (byte*)returnBlock;
		totalSize = returnBlock->TotalSize();

		auto prev = returnBlock->PrevBlock();
		if (prev && !prev->IsAllocated())
		{
			// prev block is free block, join

			auto freeBlock = (FreeBlock*)prev;
			totalSize += freeBlock->TotalSize();

			ptr = (byte*)prev;

			deletedPrev = freeBlock;
		}

		auto next = returnBlock->NextBlock();
		if ((byte*)next != m_buffer + m_size && !next->IsAllocated())
		{
			auto freeBlock = (FreeBlock*)next;
			totalSize += freeBlock->TotalSize();

			deletedNext = freeBlock;
		}
	}

private:
	inline void Insert(FreeBlock* block)
	{
		m_totalFreeBlocks++;

		auto inserted = Tree::Insert(block);

		if (inserted != block)
		{
			// one block has same size already inserted
			// add new block to same size linked-list
			block->m_next = inserted->m_next;

			if (block->m_next) block->m_next->Parent() = block;

			block->Parent() = inserted;
			block->BF() = 2;
			inserted->m_next = block;

			return;
		}

		if (m_maxSizeFreeBlock == nullptr 
			|| (m_maxSizeFreeBlock == inserted->Parent() && m_maxSizeFreeBlock->Right() == inserted))
		{
			m_maxSizeFreeBlock = inserted;
		}
	}

	inline void Erase(FreeBlock* block)
	{
		m_totalFreeBlocks--;

		if (block->BF() == 2)
		{
			auto prev = block->Parent();
			auto next = block->m_next;

			if (next) next->Parent() = prev;

			prev->m_next = next;
			return;
		}

		auto next = block->m_next;

		if (next)
		{
			// has same size block
			next->BF() = block->BF();

			next->Parent() = block->Parent();
			next->Left() = block->Left();
			next->Right() = block->Right();

			//next->Key() = block->Key();

			if (next->Left())
			{
				next->Left()->Parent() = next;
			}

			if (next->Right())
			{
				next->Right()->Parent() = next;
			}

			//if (block == m_root) m_root = next;

			if (!next->Parent())
			{
				m_root = next;
				return;
			}

			if (next->Parent()->Left() == block)
			{
				next->Parent()->Left() = next;
			}

			if (next->Parent()->Right() == block)
			{
				next->Parent()->Right() = next;
			}

			return;
		}

		if (m_maxSizeFreeBlock == block)
		{
			m_maxSizeFreeBlock = block->Parent();
		}

		Tree::Erase(block);
	}

public:
	/**
	* return 0 on out of memory
	* ----------------------------------------------------------------
	* | AllocatedBlock (16 bytes) | usable mem (size bytes)
	* ----------------------------------------------------------------
	**/
	void* Allocate(size_t size)
	{
		size = AlignSize(size + sizeof(AllocatedBlock));

		size = size <= sizeof(FreeBlock) ? AlignSize(sizeof(FreeBlock)) : size;

		auto chosenBlock = BestFit(size);

		if (!chosenBlock) return 0;

		auto next = (byte*)chosenBlock->NextBlock();
		auto prev = chosenBlock->PrevBlock();

		Erase(chosenBlock);

		//size_t chosenBlockSize = chosenBlock->TotalSize();
		size_t remainBytes = 0;
		FreeBlock* remain = 0;
		if (SplitMemory(size, chosenBlock, remain))
		{
			//remain->m_prev = chosenBlock;
			//next->m_prev = remain;

			//if (!next->IsAllocated())
			//{
			//	next->m_prev = (FreeBlock*)remain;
			//}
			//else
			//{
			if (next != m_buffer + m_size)
			{
				assert(((AllocatedBlock*)next)->IsAllocated());
				((AllocatedBlock*)next)->m_prev = (AllocatedBlock*)remain;
			}
				
			//}

			Insert(remain);
		}
		else
		{
			size = chosenBlock->TotalSize();
		}

		m_totalAllocatedBytes += size;

		auto allocatedBlock = (AllocatedBlock*)chosenBlock;
		allocatedBlock->m_totalSize = size;
		allocatedBlock->m_prev = (AllocatedBlock*)prev;

		return (allocatedBlock + 1);
	}

	void Free(void* p)
	{
		auto allocatedBlock = (AllocatedBlock*)((byte*)p - sizeof(AllocatedBlock));

		m_totalAllocatedBytes -= allocatedBlock->TotalSize();

		byte* prev = (byte*)allocatedBlock->m_prev;

		byte* ptr = 0;
		size_t size = 0;
		FreeBlock* joinedPrev = 0;
		FreeBlock* joinedNext = 0;
		ReturnAndJoin(allocatedBlock, ptr, size, joinedPrev, joinedNext);

		FreeBlock* next = (FreeBlock*)allocatedBlock->NextBlock();

		if (joinedPrev)
		{
			prev = (byte*)joinedPrev->PrevBlock();
			Erase(joinedPrev);
		}

		if (joinedNext)
		{
			next = joinedNext->NextBlock();
			Erase(joinedNext);
		}

		assert((byte*)next <= m_buffer + m_size);

		if ((byte*)next != m_buffer + m_size)
		{
			if (!next->IsAllocated())
			{
				next->m_prev = (FreeBlock*)ptr;
			}
			else
			{
				((AllocatedBlock*)next)->m_prev = (AllocatedBlock*)ptr;
			}
		}

		Insert(InitFreeBlock(ptr, size, prev));
	}

public:
	inline int64_t GetTotalAllocatedBytes()
	{
		return m_totalAllocatedBytes;
	};

	inline int64_t GetSize()
	{
		return m_size;
	};

	inline int64_t GetFragmentCount()
	{
		return m_totalFreeBlocks;
	};

	inline byte* GetBuffer()
	{
		return m_buffer;
	};

	inline size_t GetMaxFreeBlockSize()
	{
		return m_maxSizeFreeBlock->TotalSize();
	}

public:
	template <typename C>
	void SingleFreeBlockMemoryDefragment(C callback)
	{
		auto cur = m_root;

		auto curTotalSize = cur->TotalSize();

		if ((byte*)cur + curTotalSize == m_buffer + m_size) return;

		auto curOffset = ((byte*)cur - m_buffer);

		auto usedSize = m_size - (curOffset + curTotalSize);

		auto usedAddr = m_buffer + curOffset + curTotalSize;

		if constexpr (!std::is_same_v<C, nullptr_t>)
		{
			auto endAllocatedBlock = (AllocatedBlock*)(m_buffer + m_size);
			auto beginAllocatedBlock = (AllocatedBlock*)usedAddr;

			while (beginAllocatedBlock != endAllocatedBlock)
			{
				callback((byte*)beginAllocatedBlock + sizeof(AllocatedBlock),
					beginAllocatedBlock->TotalSize() - sizeof(AllocatedBlock), curTotalSize);

				beginAllocatedBlock = beginAllocatedBlock->NextBlock();
			}
		}

		Erase(cur);

		memcpy(cur, usedAddr, usedSize);

		FreeBlock* block = InitFreeBlock((byte*)cur + usedSize, curTotalSize, 0);
		
		Insert(block);
	};

	FreeBlock* FindNextFreeBlock(FreeBlock* block)
	{
		AllocatedBlock* iter = (AllocatedBlock*)block->NextBlock();
		auto end = (AllocatedBlock*)(m_buffer + m_size);
		while (iter != end && iter->IsAllocated())
		{
			iter = iter->NextBlock();
		}

		return (FreeBlock*)iter;
	};

	// callback = function(byte* originBuffer, size_t sizeofOriginBuffer, size_t offsetToDefrag)
	template <typename C>
	void MemoryDefragment(size_t limit, C callback)
	{
		auto end = (FreeBlock*)(m_buffer + m_size);

		FreeBlock* cur = 0;
		auto firstBlock = (FreeBlock*)m_buffer;

		if (firstBlock->IsAllocated())
		{
			cur = (FreeBlock*)(((AllocatedBlock*)firstBlock)->NextBlock());
		}
		else
		{
			cur = firstBlock;
		}

		auto next = FindNextFreeBlock(cur);

		size_t count = 0;
		size_t defragOffset = (byte*)cur - m_buffer;

		while (next != end)
		{
			auto curTotalSize = cur->TotalSize();
			auto nextTotalSize = next->TotalSize();
			auto curPrev = cur->PrevBlock();

			auto usedSize = (byte*)next - ((byte*)cur + curTotalSize);

			auto usedAddr = (byte*)cur + curTotalSize;

			if constexpr (!std::is_same_v<C, nullptr_t>)
			{
				auto endAllocatedBlock = (AllocatedBlock*)next;
				auto beginAllocatedBlock = (AllocatedBlock*)usedAddr;

				while (beginAllocatedBlock != endAllocatedBlock)
				{
					callback((byte*)beginAllocatedBlock + sizeof(AllocatedBlock), 
						beginAllocatedBlock->TotalSize() - sizeof(AllocatedBlock), curTotalSize);

					beginAllocatedBlock = beginAllocatedBlock->NextBlock();
				}
			}

			auto tNext = next;
			next = FindNextFreeBlock(next);

			Erase(cur);
			Erase(tNext);

			memcpy(m_buffer + defragOffset, usedAddr, usedSize);
			defragOffset += usedSize;

			FreeBlock* insertBlock = InitFreeBlock(m_buffer + defragOffset, curTotalSize + nextTotalSize, (byte*)curPrev);
			Insert(insertBlock);

			cur = insertBlock;

			if ((++count) == limit) break;
		}

		if (m_totalFreeBlocks == 1)
		{
			SingleFreeBlockMemoryDefragment(callback);
		}
	};

	// iterate over allocated blocks
	// callback = function(byte* buffer, size_t bufferSize, ...)
	// return last block
	template <typename C, typename... Args>
	byte* ForEachAllocatedBlocks(C callback, Args&&... args)
	{
		auto end = (AllocatedBlock*)(m_buffer + m_size);

		AllocatedBlock* cur = (AllocatedBlock*)m_buffer;

		byte* prev = 0;

		while (cur != end)
		{
			if (cur->IsAllocated())
			{
				while (cur != end && cur->IsAllocated())
				{
					auto next = cur->NextBlock();
					if constexpr (!std::is_same_v<C, nullptr_t>)
					{
						callback((byte*)(cur + 1), cur->TotalSize() - sizeof(AllocatedBlock), std::forward<Args>(args)...);
					}
					cur = next;
				}

				prev = (byte*)cur;
				cur = (AllocatedBlock*)(((FreeBlock*)cur)->NextBlock());
			}
			else
			{
				prev = (byte*)cur;
				cur = (AllocatedBlock*)(((FreeBlock*)cur)->NextBlock());
			}
		}

		return prev;
	};

	// callback = function(byte* oldBlock, size_t oldBlockSize, size_t offset)
	// offset = (newBuffer - oldBuffer)
	template <typename C>
	void Resize(size_t newSize, C callback)
	{
		auto alignedSize = AlignSize(newSize);

		if (alignedSize <= m_size) return;

		byte* newBuffer = (byte*)malloc(newSize);

		auto* lastBlock = ForEachAllocatedBlocks(callback, newBuffer - m_buffer);

		memcpy(newBuffer, m_buffer, m_size);

		free(m_buffer);

		AllocatedBlock* newFreeBlock = (AllocatedBlock*)(newBuffer + m_size);
		newFreeBlock->m_prev = (AllocatedBlock*)lastBlock;
		newFreeBlock->m_totalSize = alignedSize - m_size;

		m_buffer = newBuffer;
		m_size = alignedSize;

		Free(newFreeBlock);
	};

};

NAMESPACE_MEMORY_END