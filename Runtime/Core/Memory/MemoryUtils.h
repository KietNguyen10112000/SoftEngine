#pragma once

#include "TypeDef.h"
#include "ManagedHandle.h"
#include "Trace.h"

NAMESPACE_MEMORY_BEGIN


namespace MemoryUtils
{

template <typename C>
inline void ForEachTraceTable(size_t offsetFromStart, byte* addr, TraceTable* traceTable, size_t num, C callback)
{
	auto begin = traceTable->begin();
	auto end = traceTable->end();
	auto instanceSize = traceTable->instanceSize;

	for (size_t i = 0; i < num; i++)
	{
		for (auto it = begin; it != end; it++)
		{
			auto offset = it->offset;
			auto ptr = addr + offset;
			auto subTraceTable = it->traceTable;
			if (subTraceTable)
			{
				ForEachTraceTable(offsetFromStart + offset, ptr, subTraceTable, it->count, callback);
				continue;
			}

			callback(ptr, offsetFromStart + offset);
		}

		offsetFromStart += instanceSize;
		addr += instanceSize;
	}
}

template <typename C>
inline void ForEachManagedPointer(ManagedHandle* handle, C callback)
{
	auto traceTable = handle->traceTable;
	byte* ptr = handle->GetUsableMemAddress();

	auto instanceSize = traceTable->instanceSize;
	size_t num = handle->ObjectSize() / instanceSize;
	for (size_t i = 0; i < num; i++)
	{
		ForEachTraceTable(instanceSize * i, ptr, traceTable, 1, callback);
		ptr += instanceSize;
	}
}

// align num bytes to ALIGNMENT_SIZE bytes
template <size_t ALIGNMENT_SIZE>
inline size_t Align(size_t num)
{
	return (num / ALIGNMENT_SIZE + (num % ALIGNMENT_SIZE != 0)) * ALIGNMENT_SIZE;
}

}


NAMESPACE_MEMORY_END