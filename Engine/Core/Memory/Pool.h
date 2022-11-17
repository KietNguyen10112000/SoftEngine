#pragma once

#include <iostream>
#include <set>
#include <chrono>
#include <cassert>

#include "TypeDef.h"

NAMESPACE_MEMORY_BEGIN

template <size_t N, size_t CHUNK_SIZE, size_t CHUNK_ALLOC_COUNT = 1>
class Pool
{
public:
	using byte = uint8_t;
	
	struct Block
	{
		byte mem[N - sizeof(Block*)] = {};
		
		//Block* prev = 0;
		Block* next = 0;
	};
	
	struct Chunk
	{
		Block blocks[CHUNK_SIZE];		
		Chunk* next = 0;
		
		Chunk()
		{
			for (size_t i = 0; i < CHUNK_SIZE - 1; i++)
			{
				blocks[i].next = &blocks[i + 1];
			}
			
			blocks[CHUNK_SIZE - 1].next = 0;
			
			/*for (size_t i = 1; i < CHUNK_SIZE; i++)
			{
				blocks[i].prev = &blocks[i - 1];
			}*/
			//std::cout << "Chunk()\n";
		};
		
		~Chunk()
		{
			// never reach
			exit(-1);
		};
		
	};
	
public:
	Chunk* m_chunkHead = 0;
	size_t m_totalChunks = 0;
	Block* m_freeHead = 0;
	size_t m_totalAllocatedBlocks = 0;
	
public:
	Pool(size_t numChunks)
	{
		Chunk** pptr = &m_chunkHead;		
		AllocateChunks(pptr, numChunks);		
		m_freeHead = &m_chunkHead->blocks[0];
	};
	
	~Pool()
	{
		auto iter = m_chunkHead;
		while (iter)
		{
			auto temp = iter->next;
			
			free(iter);
			
			iter = temp;
		}
	};
	
private:
	inline void AllocateChunks(Chunk** pptr, size_t count)
	{
		for (size_t i = 0; i < count; i++)
		{
			auto chunk = (Chunk*)::malloc(sizeof(Chunk));
			
			new (chunk) Chunk();
			
			*pptr = chunk;			
			pptr = &chunk->next;
		}
		
		*pptr= 0;

		m_totalChunks += count;
		
		//std::cout << "AllocateChunks()\n";
	};
	
	inline void EmplaceFrontChunks()
	{
		Chunk* ptr = 0;
		AllocateChunks(&ptr, CHUNK_ALLOC_COUNT);
		auto chunk = m_chunkHead;
		m_chunkHead = ptr;
		ptr->next = chunk;
	};
	
public:
	inline byte* Allocate()
	{
		auto ret = m_freeHead;
		
		if (ret == 0)
		{
			EmplaceFrontChunks();
			m_freeHead = &m_chunkHead->blocks[0];
			ret = m_freeHead;
		}
		
		m_freeHead = ret->next;
		//m_freeHead->prev = 0;
		m_totalAllocatedBlocks++;
		return (byte*)ret;
	};
	
	inline void Deallocate(void* p)
	{
		auto returnBlock = (Block*)p;		
		//m_freeHead->prev = returnBlock;
		returnBlock->next = m_freeHead;
		m_freeHead = returnBlock;
		m_totalAllocatedBlocks--;
	};

public:
	inline size_t GetTotalChunks()
	{
		return m_totalChunks;
	}

	inline size_t GetTotalAllocatedBlocks()
	{
		return m_totalAllocatedBlocks;
	}
	
};


/// 
/// for dynamic pool size
/// 
class PoolN
{
public:
	using byte = uint8_t;

	struct Block
	{
		Block* next;
		// placeholder
		byte firstByte;
	};

	struct ChunkHeader
	{
		void* next = 0;
		void* padding;
	};

	struct Chunk
	{
		Chunk* next = 0;
		void* padding;
		// placeholder
		byte firstByte = 0;

		// size in byte
		Chunk(size_t blockCount, size_t blockSize)
		{
			auto block = (Block*)&firstByte;
			for (size_t i = 0; i < blockCount - 1; i++)
			{
				block->next = (Block*)((byte*)block + blockSize);
				block = block->next;
			}
			block->next = 0;
		};

		~Chunk()
		{
			// never reach
			exit(-1);
		};

	};

public:
	Chunk* m_chunkHead = 0;
	size_t m_totalChunks = 0;
	Block* m_freeHead = 0;
	size_t m_totalAllocatedBlocks = 0;

	size_t m_chunkSize = 0;
	size_t m_blockSize = 0;
	size_t m_allocNextChunksCount = 1;

public:
	inline PoolN() {};

	inline explicit PoolN(size_t chunkSize, size_t numChunks, size_t blockSize, size_t allocNextChunksCount = 1)
	{
		m_chunkSize = chunkSize;
		m_blockSize = blockSize;
		m_allocNextChunksCount = allocNextChunksCount;

		Chunk** pptr = &m_chunkHead;
		AllocateChunks(pptr, numChunks);
		m_freeHead = (Block*)&m_chunkHead->firstByte;
	};

	inline ~PoolN()
	{
		auto iter = m_chunkHead;
		while (iter)
		{
			auto temp = iter->next;

			free(iter);

			iter = temp;
		}
	};

private:
	inline void AllocateChunks(Chunk** pptr, size_t count)
	{
		for (size_t i = 0; i < count; i++)
		{
			auto chunk = (Chunk*)malloc(sizeof(ChunkHeader) + m_chunkSize);

			new (chunk) Chunk(m_chunkSize / m_blockSize, m_blockSize);

			*pptr = chunk;
			pptr = &chunk->next;
		}

		*pptr = 0;

		m_totalChunks += count;

		//std::cout << "AllocateChunks()\n";
	};

	inline void EmplaceFrontChunks()
	{
		Chunk* ptr = 0;
		AllocateChunks(&ptr, m_allocNextChunksCount);
		auto chunk = m_chunkHead;
		m_chunkHead = ptr;
		ptr->next = chunk;
	};

public:
	inline byte* Allocate()
	{
		auto ret = m_freeHead;

		if (ret == 0)
		{
			EmplaceFrontChunks();
			m_freeHead = (Block*)&m_chunkHead->firstByte;
			ret = m_freeHead;
		}

		m_freeHead = ret->next;
		//m_freeHead->prev = 0;
		m_totalAllocatedBlocks++;
		return (byte*)ret;
	};

	inline void Deallocate(void* p)
	{
		auto returnBlock = (Block*)p;
		//m_freeHead->prev = returnBlock;
		returnBlock->next = m_freeHead;
		m_freeHead = returnBlock;
		m_totalAllocatedBlocks--;
	};

public:
	inline size_t GetTotalChunks()
	{
		return m_totalChunks;
	}

	inline size_t GetTotalAllocatedBlocks()
	{
		return m_totalAllocatedBlocks;
	}

	inline bool IsEmpty()
	{
		return m_freeHead == 0;
	}

	inline bool IsValid()
	{
		return m_chunkSize != 0;
	}

};


// allocate only 8 bytes per allocate call
template <size_t CHUNK_SIZE, size_t CHUNK_ALLOC_COUNT = 1>
class Pool0
{
public:
	using byte = uint8_t;

	struct Block
	{
		//Block* prev = 0;
		Block* next = 0;
	};

	struct Chunk
	{
		Block blocks[CHUNK_SIZE];
		Chunk* next = 0;

		Chunk()
		{
			for (size_t i = 0; i < CHUNK_SIZE - 1; i++)
			{
				blocks[i].next = &blocks[i + 1];
			}

			blocks[CHUNK_SIZE - 1].next = 0;

			/*for (size_t i = 1; i < CHUNK_SIZE; i++)
			{
				blocks[i].prev = &blocks[i - 1];
			}*/
			//std::cout << "Chunk()\n";
		};

		~Chunk()
		{
			// never reach
			exit(-1);
		};

	};

public:
	Chunk* m_chunkHead = 0;
	Block* m_freeHead = 0;
	size_t m_totalAllocatedBlocks = 0;

public:
	Pool0(size_t numChunks)
	{
		Chunk** pptr = &m_chunkHead;
		AllocateChunks(pptr, numChunks);
		m_freeHead = &m_chunkHead->blocks[0];
	};

	~Pool0()
	{
		auto iter = m_chunkHead;
		while (iter)
		{
			auto temp = iter->next;

			free(iter);

			iter = temp;
		}
	};

private:
	inline void AllocateChunks(Chunk** pptr, size_t count)
	{
		for (size_t i = 0; i < count; i++)
		{
			auto chunk = (Chunk*)malloc(sizeof(Chunk));

			new (chunk) Chunk();

			*pptr = chunk;
			pptr = &chunk->next;
		}

		*pptr = 0;

		//std::cout << "AllocateChunks()\n";
	};

	inline void EmplaceFrontChunks()
	{
		Chunk* ptr = 0;
		AllocateChunks(&ptr, CHUNK_ALLOC_COUNT);
		auto chunk = m_chunkHead;
		m_chunkHead = ptr;
		ptr->next = chunk;
	};

public:
	// only 8 bytes usable
	inline byte* Allocate()
	{
		auto ret = m_freeHead;

		if (ret == 0)
		{
			EmplaceFrontChunks();
			m_freeHead = &m_chunkHead->blocks[0];
			ret = m_freeHead;
		}

		m_freeHead = ret->next;
		//m_freeHead->prev = 0;
		m_totalAllocatedBlocks++;
		return (byte*)ret;
	};

	inline void Deallocate(void* p)
	{
		auto returnBlock = (Block*)p;
		//m_freeHead->prev = returnBlock;
		returnBlock->next = m_freeHead;
		m_freeHead = returnBlock;
		m_totalAllocatedBlocks--;
	};

	inline size_t GetTotalAllocatedBlocks()
	{
		return m_totalAllocatedBlocks;
	}

};


///
/// MSVC STL call destructor of STLNodeAllocator without deallocate memory belong to STLNodeAllocator::m_pool
/// this is unexpected behavior, so never use STLNodeAllocator
/// 

//// local
//template <typename T, size_t POOL_ALLOC = 4*1024>
//class STLNodeAllocator
//{
//public:
//	template <class U> 
//	struct rebind
//	{
//	    using other = STLNodeAllocator<U, POOL_ALLOC>;
//	};
//	
//	using value_type = T;
//	
//	using pool_type = Pool<sizeof(T), POOL_ALLOC>;
//
//	pool_type m_pool = { 1 };
//	
//	STLNodeAllocator() = default;
//	template <class U> constexpr STLNodeAllocator(const STLNodeAllocator <U, POOL_ALLOC>&) noexcept {}
//	
//	T* allocate(size_t n)
//	{
//		//assert(n == 1);
//		auto v = sizeof(T);
//		return (T*)m_pool.Allocate();
//		//return (T*)::malloc(sizeof(T));
//	};
//	
//	void deallocate(T* p, size_t n)
//	{
//		m_pool.Deallocate((uint8_t*)p);
//	};
//	
//};

// global
template <typename T, size_t POOL_ALLOC = 4*1024>
class STLNodeGlobalAllocator
{
public:
	template <class U> 
	struct rebind
	{
	    using other = STLNodeGlobalAllocator<U, POOL_ALLOC>;
	};
	
	using value_type = T;
	
	using pool_type = Pool<sizeof(T), POOL_ALLOC>;
	
	inline static pool_type s_pool = { 1 };

	STLNodeGlobalAllocator() = default;
	template <class U> constexpr STLNodeGlobalAllocator(const STLNodeGlobalAllocator <U, POOL_ALLOC>&) noexcept {}
	
	T* allocate(size_t n)
	{
		return (T*)s_pool.Allocate();
	};
	
	void deallocate(T* p, size_t n)
	{	
		s_pool.Deallocate((byte*)p);
	};
	
};

NAMESPACE_MEMORY_END