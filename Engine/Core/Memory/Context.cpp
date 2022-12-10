#include "Context.h"

#include "Trace.h"


#include "ManagedPage.h"
#include "ManagedPool.h"
#include "GC.h"
#include "System.h"
#include "MARK_COLOR.h"

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
		if (m_timeLimit == -1)
		{
			return false;
		}

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
	assert(g_system->m_phase == GC_PHASE::MARK_PHASE || g_system->m_phase == GC_PHASE::REMARK_PHASE);
	//size_t count = 0;

	while (!m_stack.empty())
	{
		MarkState& state = m_stack.back();

		auto ptr = state.ptr;

		/*if (IsLeftMostBitEqual1((intmax_t)ptr))
		{
			m_stack.pop_back();
			continue;
		}*/

		if (state.it == 0)
		{
			auto handle = (ManagedHandle*)ptr - 1;
			auto block = (AllocatedBlock*)handle - 1;

			if (handle->marked == MARK_COLOR::WHITE)
			{
				m_stack.pop_back();
				continue;
			}
			handle->marked = MARK_COLOR::WHITE;
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
				if (traceTable->GetDynamicArraySize && m_stack.size() != 1)
				{
					/*if (m_stack.size() == 1)
					{
						state.blockEnd = 0;
					}
					else
					{
						auto size = traceTable->GetDynamicArraySize(state.ppptr);
						state.blockEnd = handle->GetUsableMemAddress() + size * traceTable->instanceSize;
					}*/
					auto size = traceTable->GetDynamicArraySize(state.ppptr);
					state.blockEnd = handle->GetUsableMemAddress() + size * traceTable->instanceSize;
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
	assert(g_system->m_phase == GC_PHASE::MARK_PHASE);
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
		auto& stack = m_localScope->stack;
		auto& stackPopLock = m_localScope->stackPopLock;

		if (m_copiedLocals.size() == 0)
		{
			stackPopLock.lock();
			//m_localScope->transactionLock.lock();

			///
			/// std::vector::size() is not atomic operator
			/// with MSVC std, vector::size() = vector::end() - vector::begin() => not atomic
			/// 
			m_localScope->stackPushLock.lock();
			auto size = stack.size();
			m_localScope->stackPushLock.unlock();

			m_copiedLocals.resize(size);
			auto buf = stack.data();

			for (size_t i = 0; i < size; i++)
			{
				m_copiedLocals[i] = *(buf[i]);
			}

			//m_localScope->transactionLock.unlock();
			stackPopLock.unlock();
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
	}
}

void gc::Context::RemarkPhase()
{
	assert(g_system->m_phase == GC_PHASE::REMARK_PHASE);
	//std::cout << "RemarkPhase [" << ThreadID::Get() << "]\n";

	if (m_localScope)
	{
		// we must stop-the-world to process transactions (no way here)
		// done re-mark phase for this local scope
		// process transactions
		m_localScope->transactionLock.lock();
		m_localScope->isRecordingTransactions = false;

		// fake batch size so that mark process will never be paused
		auto tempBatchSize = m_batchSize;
		auto tempT0 = m_t0;
		m_batchSize = -1;
		m_t0 = -1;

		auto& transactions = m_localScope->transactions;
		auto size = transactions.size();
		auto buf = transactions.data();

		while (m_localScopeAllocatedIdx != size && m_isPaused == false)
		{
			auto& r = buf[m_localScopeAllocatedIdx];
			if (*r.pptr == r.ptr) // valid transaction
			{
				m_stack.push_back({ 0, 0, 0, r.ptr, 0, 0 });
				Mark();
			}
			m_localScopeAllocatedIdx++;
		}


		assert(m_isPaused == false);
		transactions.clear();
		
		m_batchSize = tempBatchSize;
		m_t0 = tempT0;
		m_localScope->transactionLock.unlock();
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
	assert(g_system->m_phase == GC_PHASE::SWEEP_PHASE);

	m_page->m_lock.lock();

	auto end = (AllocatedBlock*)(m_sweepEnd);

	auto& cur = m_page->m_sweepIt;
	
	///
	/// memory map of ManagedPage
	/// ab -> allocated-block
	/// fb -> free-block
	/// 
	///                   +--- sweepIt     
	///                   v
	/// +--------------------------------------------+
	/// | ab | fb | ab | ab | ab | fb | ab | fb | ab |
	/// +--------------------------------------------+
	/// 
	/// new allocated block created on the paused sweeping page has 2 cases:
	/// + before sweepIt, mark with BLACK color
	/// + after sweepId, mark with GRAY color
	/// -> so we can incrementally sweep a ManagedPage
	/// 

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

		if (handle->marked == MARK_COLOR::BLACK)
		{
			CallDestructor(handle);
			/*DEBUG_CODE(
				if (handle->traceTable)
					CONSOLE_LOG() << "free memory of \"" << handle->traceTable->className << "\"\n";
			)*/
			m_page->Deallocate(handle + 1);
		}
		else
		{
			handle->marked = MARK_COLOR::BLACK;
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
				m_isPaused = true;
				break;
			}
		}
	}

end:
#ifdef _DEBUG
	if (m_isPaused == false)
	{
		m_page->ForEachAllocatedBlocks([](ManagedHandle* handle)
			{
				assert(handle->marked == MARK_COLOR::BLACK);
			}
		);
	}
#endif // _DEBUG

	m_page->m_lock.unlock();
}

void gc::Context::SweepPool()
{
	assert(g_system->m_phase == GC_PHASE::SWEEP_PHASE);

	m_pool->m_lock.lock();

	auto& forward = m_pool->m_sweepIt;
	auto& backward = m_pool->m_sweepBackwardIt;
	if (backward && forward == backward) backward = backward->prev;

	/// 
	/// memory map of ManagedPool
	/// ab -> allocated-block
	/// 
	///                                 backward ---+  +--- forward
	///           +----- head                       v  v                                  +--- tail
	///           v						          <--||-->                                v
	/// null <- [ab] <-> [ab] <-> [ab] <-> [ab] <-> [ab] <-> [ab] <-> [ab] <-> [ab] <-> [ab] -> null
	/// 
	/// we do sweep from forward to tail and re-paint all allocated-block from backward to head with BLACK
	/// with backward and forward we can incrementally sweep a ManagedPool
	/// 

	while (forward)
	{
		auto next = forward->next;

		ManagedHandle* handle = (ManagedHandle*)(forward + 1);

		if (handle->marked == MARK_COLOR::BLACK)
		{
			CallDestructor(handle);
			/*DEBUG_CODE(
				if (handle->traceTable)
					CONSOLE_LOG() << "free memory of \"" << handle->traceTable->className << "\"\n";
			)*/
			m_pool->Deallocate(handle + 1);
		}
		else
		{
			handle->marked = MARK_COLOR::BLACK;
		}

		forward = next;

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

	if (m_isPaused == false && backward)
	{
		while (backward != 0)
		{
			ManagedHandle* handle = (ManagedHandle*)(backward + 1);
			handle->marked = MARK_COLOR::BLACK;
			backward = backward->prev;
		}
	}


#ifdef _DEBUG
	if (m_isPaused == false)
	{
		m_pool->ForEachAllocatedBlocks([](ManagedHandle* handle)
			{
				assert(handle->marked == MARK_COLOR::BLACK);
			}
		);
	}
#endif // _DEBUG

	//std::cout << "Sweeped pool id : " << (int)m_pool->m_id << " PAGE_SIZE : " << m_pool->m_PAGE_SIZE << "\n";

	m_pool->m_lock.unlock();
}

void gc::Context::SweepPhase()
{
	//std::cout << "SweepPhase [" << ThreadID::Get() << "]\n";

	if (m_page && m_page->m_sweepIt != m_sweepEnd)
	{
		SweepPage();
	}

	if (!m_isPaused && m_pool)
	{
		SweepPool();
	}
}

void gc::Context::Resume(size_t timeLimit, size_t flags)
{
	m_t0 = Clock::ns::now();
	m_timeLimit = timeLimit;
	m_isPaused = false;
	m_counter = 0;
	m_resumeFlags = flags;
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
		//m_sweepCycles++;
		//g_system->EndGCCycle();
		//std::cout << ThreadID::Get() << "  ======================================================\n";

		//CONSOLE_LOG() 
		//	<< "============ End GC cycle by ThreadID [" << ThreadID::Get() << "] =============\n";
		g_system->m_phase = GC_PHASE::IDLE_PHASE;
	}
}

}

NAMESPACE_MEMORY_END