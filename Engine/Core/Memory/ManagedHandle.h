#pragma once

#include "TypeDef.h"
#include "Pool.h"

NAMESPACE_MEMORY_BEGIN

//using ManagedHandlesPool = Pool0<128 * KB>;

//struct ManagedHandleUsableMem
//{
//	byte* startUsableMemAddr = 0;
//};

struct TraceTable;

using Dtor = void(*)(void*);

struct ManagedHandle
{
	TraceTable* traceTable = 0;

	// how many bytes used to align memory size, to call destructor on array
	uint16_t paddingBytes = 0;

	// used by ManagedPool
	//uint16_t poolPageSize = 0;
	byte poolId = 0;
	byte log2OfPoolPageSize = 0;

	union
	{
		byte pageId = 0;

		// used by ManagedPage
		byte pageHint;
	};

	union
	{
		//byte poolId = 0;

		// used by ManagedPool
		byte poolHint;
	};

	// gc
	byte marked = 0;

	// to make object stable
	// if stableValue == 0 => gc object
	// else => stable object
	// all stable objects have same stableValue will be cleaned up at the same time
	byte stableValue;

	inline byte* GetUsableMemAddress()
	{
		return (byte*)(this + 1);
	}

	/*inline size_t GetUsableMemSize()
	{
		AllocatedBlock* block = (AllocatedBlock*)((byte*)this - sizeof(AllocatedBlock));
		return block->TotalSize() - sizeof(AllocatedBlock) - sizeof(ManagedHandle);
	}*/

	inline bool IsLargeObject()
	{
		return pageId != 0xff;
	}

	// include AllocatedBlock/PoolLink + ManagedHandle
	inline size_t TotalSize()
	{
		if (pageId == 0xff)
		{
			//return poolPageSize * (poolHint + 1);
			return ((size_t)1 << log2OfPoolPageSize) * (poolHint + 1);
		}
		return ((AllocatedBlock*)(this - 1))->TotalSize();
	}

	inline size_t UsableSize()
	{
		return TotalSize() - 32;
	}

	inline size_t ObjectSize()
	{
		return TotalSize() - paddingBytes - 32;
	}

	inline byte* Begin()
	{
		return GetUsableMemAddress();
	}

	inline byte* End()
	{
		return (byte*)(this - 1) + TotalSize() - paddingBytes;
	}

	/*inline byte** GetManagedHandleUsableMem()
	{
		return &((ManagedHandleUsableMem*)poolBlock)->startUsableMemAddr;
	}*/
};

NAMESPACE_MEMORY_END