#include "Context.h"

#include "Trace.h"


#include "ManagedPage.h"
#include "ManagedPool.h"
#include "GC.h"
#include "System.h"

#include "Core/Thread/ThreadID.h"
#include "Core/Time/Clock.h"

NAMESPACE_MEMORY_BEGIN

namespace gc
{

extern System* g_system;

bool gc::Context::Handle()
{
	if (m_sharedHandle->m_isSelfHandle)
	{
		if (Clock::ns::now() - m_t0 > m_timeLimit)
		{
			return true;
		}
	}
	else
	{
		m_sharedHandle->Control(this);

		if (m_sharedHandle->m_isPaused)
		{
			return true;
		}
	}

	return false;
}

Context::Context(ContextSharedHandle* hd)
{
	m_stack.reserve(16);
	m_sharedHandle = hd;

	m_copiedLocals.reserve(1024);
}

Context::~Context()
{
}

void gc::Context::Mark()
{
	//size_t count = 0;

	while (!m_stack.empty())
	{
		MarkState& state = m_stack.back();

		auto ptr = state.ptr;

		if (IsLeftMostBitEqual1((intmax_t)ptr))
		{
			m_stack.pop_back();
			continue;
		}

		if (state.it == 0)
		{
			auto handle = (ManagedHandle*)ptr - 1;
			auto block = (AllocatedBlock*)handle - 1;

			if (handle->marked != 0)
			{
				m_stack.pop_back();
				continue;
			}
			handle->marked = 1;
			//count++;
			
			//std::cout << "Mark\n";

			// get data of state.it, state.end
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
				if (traceTable->GetDynamicArraySize)
				{
					if (m_stack.size() == 1)
					{
						state.blockEnd = 0;
					}
					else
					{
						auto size = traceTable->GetDynamicArraySize(state.ppptr);
						state.blockEnd = handle->GetUsableMemAddress() + size * traceTable->instanceSize;
					}
				}
				else
				{
					state.blockEnd = (byte*)block + handle->TotalSize() - handle->paddingBytes;
				}
			}
			else
			{
				//state.blockEnd = 0;
				m_stack.pop_back();
				continue;
			}

			//====================================================
			// timing
			if (((++m_counter) % m_batchSize) == 0)
			{
				m_counter = 0;
				if (Handle())
				{
					m_isPaused = true;
					break;
				}
			}
			//====================================================
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
				//continue;
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
				//continue;
			}

			state.it++;
			continue;
		}

		//state.ptr += state.traceTable->instanceSize;
		state.ptr += state.traceTable->instanceSize;
		if (state.ptr /*+ state.traceTable->instanceSize*/ < state.blockEnd)
		{
			state.it = state.traceTable->begin();
			continue;
		}

		m_stack.pop_back();
	}

	//std::cout << "[MARKED]:\t" << count << "\n";
}

void gc::Context::MarkPhase()
{
	//std::cout << "MarkPhase [" << ThreadID::Get() << "]\n";

	while (m_rootIt != m_rootEnd && m_isPaused == false)
	{
		if (*m_rootIt && *(*m_rootIt))
		{
			m_stack.push_back({ 0, 0, 0, *(*m_rootIt), 0, 0 });
			Mark();
		}

		m_rootIt++;
	}

	if (m_localScope)
	{
		if (m_copiedLocals.size() == 0)
		{
			auto& stack = m_localScope->stack;
			auto& popLock = m_localScope->popLock;

			popLock.lock();

			auto size = stack.size();
			m_copiedLocals.resize(size);

			for (size_t i = 0; i < size; i++)
			{
				m_copiedLocals[i] = *stack[i];
			}

			popLock.unlock();
		}
		
		const auto size = m_copiedLocals.size();
		auto* buf = m_copiedLocals.data();
		while (m_localScopeAllocatedIdx < size && m_isPaused == false)
		{
			auto r = buf[m_localScopeAllocatedIdx];
			if (r != 0)
			{
				m_stack.push_back({ 0, 0, 0, r, 0, 0 });
				Mark();
			}
			m_localScopeAllocatedIdx++;
		}

		if (m_isPaused == false)
		{
			m_copiedLocals.clear();
		}

		//lock.unlock();
	}
}

void gc::Context::RemarkPhase()
{
	//std::cout << "RemarkPhase [" << ThreadID::Get() << "]\n";

	if (m_localScope)
	{
		// done mark phase for this local scope
		// process transactions
		assert(m_localScope->isRecordingTransactions == false);
		auto& transactions = m_localScope->transactions;
		//auto& tlock = m_localScope->tLock;

		//tlock.lock();
		auto size = transactions.size();
		auto buf = transactions.data();

		while (m_localScopeAllocatedIdx != size && m_isPaused == false)
		{
			auto& r = buf[m_localScopeAllocatedIdx];

			//if (r.pptr == 0)
			//{
			//	// force remark
			//	ManagedHandle* handle = (ManagedHandle*)r.ptr - 1;
			//	handle->marked = 0;
			//	m_stack.push_back({ 0, 0, 0, r.ptr, 0, 0 });
			//	Mark();
			//}
			//else 
			if (/**r.pptr == */r.ptr) // valid transaction
			{
				ManagedHandle* handle = (ManagedHandle*)r.ptr - 1;
				handle->marked = 0;
				m_stack.push_back({ 0, 0, 0, r.ptr, 0, 0 });
				Mark();
			}
			m_localScopeAllocatedIdx++;
		}

		if (!m_isPaused)
		{
			/*if (m_localScope->bindedTransaction && (*m_localScope->bindedTransaction))
			{
				m_stack.push_back({ 0, 0, 0, *m_localScope->bindedTransaction, 0, 0 });
				Mark();
			}*/

			transactions.clear();
		}
		
		//tlock.unlock();
	}
}

void gc::Context::CallDestructor(ManagedHandle* handle)
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
	/*if (handle->traceTable->instanceSize < 64)
	{
		::memset(handle->GetUsableMemAddress(), 0, handle->ObjectSize());
	}*/
}

void gc::Context::SweepPage()
{
	m_page->m_lock.lock();

	auto end = (AllocatedBlock*)(m_sweepEnd);

	auto& cur = m_page->m_sweepIt;
	//auto prev = cur;

	while (cur != end)
	{
		if (!cur->IsAllocated())
		{
			cur = (AllocatedBlock*)(((FreeBlock*)cur)->NextBlock());
			assert(cur <= end);
			//prev = cur;
			continue;
		}

		ManagedHandle* handle = (ManagedHandle*)(cur + 1);

		auto next = cur->NextBlock();
		assert(next <= end);

		if (next != end && !next->IsAllocated())
		{
			next = (AllocatedBlock*)(((FreeBlock*)next)->NextBlock());
			assert(next <= end);
		}

		if (handle->marked == 0)
		{
			CallDestructor(handle);
			/*DEBUG_CODE(
				if (handle->traceTable)
					CONSOLE_LOG() << "free memory of \"" << handle->traceTable->className << "\"\n";
			)*/
			m_page->Deallocate(handle + 1);
			//std::cout << "Sweep\n";
		}
		else //if (m_sharedHandle->m_sweepCycles % handle->marked == 0) // for generation gc
		{
			handle->marked = 0;
		}

		assert(next == end || next->IsAllocated());
		cur = next;
		//prev = cur;


		//====================================================
		if (((++m_counter) % m_batchSize) == 0)
		{
			m_counter = 0;
			if (Handle())
			{
				//std::cout << "SweepPage paused\n";
				m_isPaused = true;
				break;
			}
		}
	}

	if (!m_isPaused)
	{
		m_page->ForEachAllocatedBlocks([](ManagedHandle* handle)
			{
				//handle->marked = 0;
				assert(handle->marked == 0);
			}
		);
	}

	m_page->m_lock.unlock();
}

void gc::Context::SweepPool()
{
	m_pool->m_lock.lock();

	if (m_pool->m_sweepBackwardIt)
	{
		auto& backward = m_pool->m_sweepBackwardIt;
		backward = backward->prev;
		while (backward != 0)
		{
			ManagedHandle* handle = (ManagedHandle*)(backward + 1);
			handle->marked = 0;
			backward = backward->prev;
		}
	}

	auto& it = m_pool->m_sweepIt;

	while (it)
	{
		auto next = it->next;

		ManagedHandle* handle = (ManagedHandle*)(it + 1);

		if (handle->marked == 0)
		{
			CallDestructor(handle);
			/*DEBUG_CODE(
				if (handle->traceTable)
					CONSOLE_LOG() << "free memory of \"" << handle->traceTable->className << "\"\n";
			)*/
			m_pool->Deallocate(handle + 1);
		}
		else //if (m_sharedHandle->m_sweepCycles % handle->marked)
		{
			handle->marked = 0;
		}

		it = next;

		//====================================================
		if (((++m_counter) % m_batchSize) == 0)
		{
			m_counter = 0;
			if (Handle())
			{
				m_isPaused = true;
				break;
			}
		}
	}

	if (!m_isPaused)
	{
		m_pool->ForEachAllocatedBlocks([](ManagedHandle* handle) 
			{
				//handle->marked = 0;
				assert(handle->marked == 0);
			}
		);
	}

	m_pool->m_lock.unlock();
}

void gc::Context::SweepPhase()
{
	//std::cout << "SweepPhase [" << ThreadID::Get() << "]\n";

	if (m_page && m_page->m_sweepIt != m_sweepEnd)
	{
		SweepPage();
	}

	if (!m_isPaused && m_pool && m_pool->m_sweepIt != 0)
	{
		SweepPool();
	}
}

void gc::Context::Resume(size_t timeLimit)
{
	m_t0 = Clock::ns::now();
	m_timeLimit = timeLimit;
	m_isPaused = false;
	m_counter = 0;
}

void gc::Context::Run()
{
	if (IsInMarkPhase())
	{
		m_batchSize = m_sharedHandle->m_markBatchSize;
		// continue
		if (!m_stack.empty())
		{
			Mark();
		}
		
		MarkPhase();

		if (!m_isPaused)
		{
			m_sharedHandle->EndMark(this);
		}
	}
	else if (IsInRemarkPhase())
	{
		m_batchSize = m_sharedHandle->m_markBatchSize;

		// continue
		if (!m_stack.empty())
		{
			Mark();
		}

		RemarkPhase();

		if (!m_isPaused)
		{
			m_sharedHandle->EndRemark(this);
		}
	}
	else
	{
		m_batchSize = m_sharedHandle->m_sweepBatchSize;

		SweepPhase();

		if (!m_isPaused)
		{
			m_sharedHandle->EndSweep(this);
		}
	}
}

bool ContextSharedHandle::Control(Context* ctx)
{
	return false;
}

void ContextSharedHandle::EndMark(Context* ctx)
{
	/*if (m_markCycles != m_sweepCycles)
	{
		return false;
	}*/

	// atomic operator
	// thread-safe
	auto ret = (++m_counter);
	if (ret == m_targetCounter)
	{
		g_system->InitializeNewMarkCycle(true);
	}
}

void gc::ContextSharedHandle::EndRemark(Context* ctx)
{
	auto ret = (++m_counter);
	if (ret == m_targetCounter)
	{
		g_system->InitializeNewSweepCycle();
	}
}

void ContextSharedHandle::EndSweep(Context* ctx)
{
	/*if (m_markCycles == m_sweepCycles)
	{
		return;
	}*/

	if (m_totalAllocatedBytes != 0)
	{
		m_totalAllocatedBytes->fetch_sub(ctx->m_totalReclaimedBytes);
		ctx->m_totalReclaimedBytes = 0;
	}

	// atomic operator
	// thread-safe
	auto ret = (++m_counter);
	if (ret == m_targetCounter)
	{
		m_sweepCycles++;
		//g_system->EndGCCycle();
		//std::cout << ThreadID::Get() << "  ======================================================\n";

		//CONSOLE_LOG() 
		//	<< "============ End GC cycle by ThreadID [" << ThreadID::Get() << "] =============\n";
	}
}

}

NAMESPACE_MEMORY_END