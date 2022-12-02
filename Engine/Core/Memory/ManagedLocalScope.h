#pragma once

#include <thread>
#include <vector>
#include <cassert>

#include "Core/Thread/Spinlock.h"
#include "Core/Thread/ThreadLimit.h"
#include "Core/Fiber/FiberInfo.h"
#include "Core/Thread/Spinlock.h"

#include "ManagedHandle.h"
#include "GC.h"
#include "NewMalloc.h"
#include "MARK_COLOR.h"


NAMESPACE_MEMORY_BEGIN

#ifndef MANAGED_LOCAL_SCOPE_GC_TIME
// 1600 ns ~ 0.0016 ms
#define MANAGED_LOCAL_SCOPE_GC_TIME 1600
#endif


namespace gc
{
	class ContextSharedHandle;
	class Context;
	class System;
}

// thread-safe class
class ManagedLocalScope
{
public:
	struct Transaction
	{
		byte** pptr;
		byte* ptr;
	};

	// defer stack buffer allocator (for concurent mark phase)
	template <typename _T>
	class Allocator
	{
	public:
		using value_type = _T;

		Allocator() = default;
		template <class U> constexpr Allocator(const Allocator <U>&) noexcept {}

		[[nodiscard]] _T* allocate(std::size_t n)
		{
			// direct alloc
			return (_T*)malloc(sizeof(_T) * n);
		}

		void deallocate(_T* p, std::size_t n) noexcept
		{
			// defer free (will free at new GC cycle)
			gc::internal::DeferFree(p);
		}
};

//#ifndef _DEBUG
//private:
//#endif // _DEBUG

	friend class ManagedHeap;
	friend class ManagedPage;
	friend class gc::ContextSharedHandle;
	friend class gc::Context;
	friend class gc::System;

	template <typename T>
	friend class ManagedPtr;
	template <typename T>
	friend class ManagedPointer;

	using Stack = std::vector<byte**, Allocator<byte**>>;
	using List = std::vector<Transaction, Allocator<Transaction>>;
	using CheckPointStack = std::vector<size_t, STDAllocatorMalloc<size_t>>;

	struct S
	{
		Stack stack;
		CheckPointStack checkpoints;

		bool isRegistered = false;
		bool isRecordingTransactions = false;

		// lock guard for stack pop operator
		Spinlock stackLock;

		// lock guard for transactions
		Spinlock transactionLock;

		// list of transactions during mark phase
		List transactions;

		inline void Log()
		{
			std::cout << "[   LOG    ]\t\t" << stack.size() << " objects live on stack" << "\n";
		}
	};

	API static S** GetManagedLocalScope();

	// call before main
	API static S** GetManagedLocalScopeBootPhase();
	API static void ReleaseManagedLocalScopeBootPhase(S*);
	API static bool IsManagedLocalScopeBootPhase(S*);

	inline static thread_local S** s = 0;

	/*inline static void WaitForLock(spinlock& lock)
	{
		while (lock.try_lock() == false)
		{
			if (gc::Resume(MANAGED_LOCAL_SCOPE_GC_TIME) == gc::GC_RETURN::EMPTY_TASK)
			{
				std::this_thread::yield();
			}
		}
	}*/

protected:
	friend class Thread;

	API static ManagedLocalScope::S* s_managedLocalScopeThreads[ThreadLimit::MAX_THREADS];
	API static ManagedLocalScope::S s_managedLocalScopeFibers[ThreadLimit::MAX_THREADS + FiberInfo::FIBERS_COUNT];

public:
	static void Initialize();

	// call inside main
	static void InitializeForThisThread()
	{
		if (s == 0 || IsManagedLocalScopeBootPhase(*s))
		{
			FlushStackForGC();
			s = GetManagedLocalScope();
		}
	}

private:
	class ManagedScopeInitializer
	{
	private:
		inline static int Boot()
		{
			if (ManagedLocalScope::s == 0) ManagedLocalScope::s = GetManagedLocalScopeBootPhase();
			return 0;
		}
		inline static int s_boot = Boot();
	};

public:
	inline static void AddCheckpoint()
	{
		auto local = (*s);
		local->checkpoints.push_back(local->stack.size());
	}

	inline static void JumpbackToCheckpoint()
	{
		auto local = (*s);
		auto size = local->checkpoints.back();
		local->checkpoints.pop_back();

		local->stackLock.lock();
		local->stack.resize(size);
		local->stackLock.unlock();
	}

	inline static void FlushStackForGC()
	{
		if (s == 0) return;

		auto& stack = (*s)->stack;
		if (!stack.empty())
		{
			gc::RegisterRoots(&stack[0], stack.size());
		}
	}

	inline static void Push(byte** p)
	{
		auto scope = *s;
		//scope->stackLock.lock();
		scope->stack.push_back(p);
		//scope->stackLock.unlock();

		if (*p != 0)
		{
			assert(0);
			Transaction(p, *p);
		}
	}

	inline static void Pop(byte** p)
	{
		auto scope = *s;
		if (scope->stack.size() == 0) return;

		scope->stackLock.lock();
		if (scope->stack.back() == p) scope->stack.pop_back();
		else
		{
			auto& back = scope->stack.back();
			// return ManagedPtr case
			if (*(&back - 1) == p)
			{
				Transaction(*(&back - 1), *back);
				*(&back - 1) = back;
				scope->stack.pop_back();
			}
		}
		scope->stackLock.unlock();
	}

	// transaction during mark phase
	inline static void Transaction(byte** p, byte* ptr)
	{
		auto* scope = *s;

		auto handle = ((ManagedHandle*)ptr) - 1;
		if (handle->marked == MARK_COLOR::WHITE || handle->marked == MARK_COLOR::TRANSACTION_COLOR /*|| handle->marked == MARK_COLOR::GRAY*/)
		{
			return;
		}

		//scope->transactionLock.lock();
		while (!scope->transactionLock.try_lock())
		{
			gc::Run(16'000'000);
		}

		if (scope->isRecordingTransactions == true)
		{
			/*if (handle->marked == MARK_COLOR::GRAY)
			{
				DEBUG_CODE(CONSOLE_LOG() << "Transaction[" << p << "]\n";);
			}*/

			//DEBUG_CODE(CONSOLE_LOG() << "Transaction[" << p << "]\n";);
			handle->marked = MARK_COLOR::TRANSACTION_COLOR;
			scope->transactions.push_back({ p, ptr });
		}	
		scope->transactionLock.unlock();
	}

	/*inline static void Transaction(byte* p)
	{
		if ((*s)->isRecordingTransactions == true)
		{
			//DEBUG_CODE(CONSOLE_LOG() << "Transaction[" << p << "]\n";);
			(*s)->transactions.push_back({ 0, p });
		}
	}*/

	template <typename C>
	inline static void ForEach(C callback)
	{
		for (auto& v : (*s)->stack)
		{
			callback(v.ptr);
		}
	}

};

NAMESPACE_MEMORY_END