#include "ContextManual.h"

#include "GC.h"
#include "System.h"

#include "ManagedHeap.h"

NAMESPACE_MEMORY_BEGIN

namespace gc
{

ContextManual::ContextManual()
{
	m_stack.reserve(KB);
}

// just copy from Context
void ContextManual::Mark(byte MARK_VALUE)
{
	while (!m_stack.empty())
	{
		MarkState& state = m_stack.back();

		auto ptr = state.ptr;

		if (state.it == 0)
		{
			auto handle = (ManagedHandle*)ptr - 1;
			auto block = (AllocatedBlock*)handle - 1;

			if (handle->marked == MARK_VALUE)
			{
				m_stack.pop_back();
				continue;
			}
			handle->marked = MARK_VALUE;

			TraceTable* traceTable = handle->traceTable;

			assert(traceTable != 0);

			if (traceTable->tableSize != 0)
			{
				// to support on/off trace
				if (traceTable->GetTraceState && traceTable->GetTraceState(state.ptr) == false)
				{
					m_stack.pop_back();
					continue;
				}

				state.it = traceTable->begin();
				state.traceTable = traceTable;

				// to support dynamic array
				if (traceTable->GetDynamicArraySize && m_stack.size() != 1)
				{
					auto size = traceTable->GetDynamicArraySize(state.ppptr);
					if (size == 0)
					{
						m_stack.pop_back();
						continue;
					}
					state.blockEnd = handle->GetUsableMemAddress() + size * traceTable->instanceSize;
				}
				else
				{
					state.blockEnd = (byte*)block + handle->TotalSize() - handle->paddingBytes;
				}
			}
			else
			{
				m_stack.pop_back();
				continue;
			}
		}

		if (state.it != state.traceTable->end())
		{
			if (state.it->traceTable == 0)
			{
				auto ppptr = (byte**)(ptr + state.it->GetOffset());
				auto nextManagedPtr = *ppptr;
				if (nextManagedPtr)
				{
					m_stack.push_back({ 0, 0, 0, nextManagedPtr, ppptr, 0 });
				}
			}
			else
			{
				auto ppptr = (byte**)(ptr + state.it->GetOffset());
				auto traceTable = state.it->traceTable;

				// to support on/off trace
				if (traceTable->GetTraceState == 0 || traceTable->GetTraceState((byte*)ppptr) == true)
				{
					auto blockEnd = (byte*)ppptr + state.it->count * traceTable->instanceSize;
					m_stack.push_back({ traceTable->begin(), traceTable, blockEnd, (byte*)ppptr, ppptr, 0 });
				}
			}

			state.it++;
			continue;
		}

		state.ptr += state.traceTable->instanceSize;
		if (state.ptr < state.blockEnd)
		{
			state.it = state.traceTable->begin();
			continue;
		}

		m_stack.pop_back();
	}
}

void ContextManual::CallDestructor(ManagedHandle* handle)
{
	auto dtor = handle->traceTable->dtor;
	if (dtor)
	{
		byte* it = handle->GetUsableMemAddress();
		byte* end = handle->End();
		auto stride = handle->traceTable->instanceSize;

		while (it != end)
		{
			dtor(it);
			it += stride;
			assert(it <= end);
		}
	}
}

void ContextManual::DoGC(byte MARK_VALUE, System* system, byte* RESET_MARK_VALUES, ManagedHeap** heaps, size_t heapCount)
{
	gc::BlockGC(true);

	auto& localScopes = system->m_localScopes;
	auto localsCount = system->m_localScopesCount;

	// block all other threads
	for (size_t i = 0; i < localsCount; i++)
	{
		auto& localScope = localScopes[i];
		localScope->transactionLock.lock();
	}

	assert(system->m_phase == GC_PHASE::IDLE_PHASE);

	// mark phase
	{
		for (size_t i = 0; i < system->m_rootsCount; i++)
		{
			auto root = system->m_roots[i];
			if (root && *root)
			{
				m_stack.push_back({ 0, 0, 0, *root, 0, 0 });
				Mark(MARK_VALUE);
			}
		}

		for (size_t i = 0; i < localsCount; i++)
		{
			auto& stack = localScopes[i]->stack;
			for (auto& v : stack)
			{
				if (*v != 0)
				{
					m_stack.push_back({ 0, 0, 0, *v, 0, 0 });
					Mark(MARK_VALUE);
				}
			}

		}

		byte* ptr = 0;
		while (system->m_crossBoundaries.try_dequeue(ptr))
		{
			system->m_trackedCrossBoundaries.push_back(ptr);
		}

		if (system->m_trackedCrossBoundaries.size() > 0)
		{
			for (auto& v : system->m_trackedCrossBoundaries)
			{
				if (v != 0)
				{
					m_stack.push_back({ 0, 0, 0, v, 0, 0 });
					Mark(MARK_VALUE);
				}
			}
		}
	}
	

	// sweep phase
	{
		for (size_t i = 0; i < heapCount; i++)
		{
			auto heap = heaps[i];
			auto& pages = heap->m_pages;
			auto pools = &heap->m_tinyObjectPools[0];

			const auto RESET_VALUE = RESET_MARK_VALUES[i];

			for (size_t j = 0; j < ManagedHeap::TOTAL_POOLS; j++)
			{
				auto pool = pools[j];
				if (pool)
				{
					pool->m_lock.lock();
					pool->ForEachAllocatedBlocks([this, pool, MARK_VALUE, RESET_VALUE](ManagedHandle* handle)
						{
							assert(handle->marked != MARK_COLOR::WHITE);

							if (handle->marked == MARK_COLOR::GRAY)
							{
								return;
							}

							if (handle->marked == MARK_VALUE)
							{
								handle->marked = RESET_VALUE;
								return;
							}

							CallDestructor(handle);
							pool->Deallocate(handle + 1);
						}
					);
					pool->m_lock.unlock();
				}
			}

			for (auto& row : pages)
			{
				for (auto& page : row)
				{
					if (page)
					{
						page->m_lock.lock();
						page->ForEachAllocatedBlocks([this, page, MARK_VALUE, RESET_VALUE](ManagedHandle* handle)
							{
								assert(handle->marked != MARK_COLOR::WHITE);

								if (handle->marked == MARK_COLOR::GRAY)
								{
									return;
								}

								if (handle->marked == MARK_VALUE)
								{
									handle->marked = RESET_VALUE;
									return;
								}

								CallDestructor(handle);
								page->Deallocate(handle + 1);
							}
						);
						page->m_lock.unlock();
					}
				}
			}
		}
	}

	// unblock all other threads
	for (size_t i = 0; i < localsCount; i++)
	{
		auto& localScope = localScopes[i];
		localScope->transactionLock.unlock();
	}

	gc::BlockGC(false);
}

}

NAMESPACE_MEMORY_END