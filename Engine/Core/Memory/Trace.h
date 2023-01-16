#pragma once

#include <array>
#include <atomic>
#include <vector>

#include "Core/Thread/Spinlock.h"

#include "TypeDef.h"
#include "Pool.h"
#include "SimpleStackBasedAllocator.h"
#include "NewMalloc.h"


#if defined _DEBUG || defined _REL_WITH_DEB_INFO
#define _TRACE_DEBUG
#endif // _DEBUG



NAMESPACE_MEMORY_BEGIN

#define TRACEABLE_FRIEND()		\
friend class Tracer;			\
friend struct TraceTable;

using Dtor = void (*)(void*);
struct TraceTable;

constexpr static size_t TRACE_TABLE_MAX_SIZE = 256;

struct TraceTableElement
{
	size_t offset;

	// does element has sub traceTable
	// traceable object contains traceable object
	TraceTable* traceTable;

	// number of sequence managed pointer
	size_t count;
	
	inline size_t GetOffset()
	{
		return offset;
	}
};

struct TraceTableView
{
#ifdef _TRACE_DEBUG
	TraceTableView* self;
	const char* className = 0;
#endif // _DEBUG

	Dtor dtor = 0;

	size_t instanceSize;
	size_t (*GetDynamicArraySize)(void* parent) = 0;
	// on/off trace
	bool   (*GetTraceState)(void* self) = 0;

	size_t tableSize;
	TraceTableElement table[TRACE_TABLE_MAX_SIZE];

	void Print()
	{
#ifdef _TRACE_DEBUG
		std::cout << className << "[size = " << instanceSize << ", table:\n| offset\t| count\t|";
#else
		std::cout << "[size = " << instanceSize << ", table:\n| offset\t| count\t|";
#endif
		for (size_t i = 0; i < tableSize - 1; i++)
		{
			std::cout << "| " << table[i].offset << "\t| " << table[i].count << "\t|\n";
		}
		std::cout << "]";
	}
};

/**
* -----------------------------------------------------
*  instanceSize | tableSize = N | array with size = N
* -----------------------------------------------------
*/
// each Traceable class has only 1 TraceTable
struct TraceTable
{
	//inline static Page* s_page;
	inline static SimpleStackBasedAllocator* s_allocator;
	inline static spinlock s_lock;

#ifdef _TRACE_DEBUG
	TraceTableView* view;
	const char* className;
#endif

	Dtor dtor = 0;

	// sizeof each class instance, to detect array, ...
	size_t instanceSize;

	size_t (*GetDynamicArraySize)(void* parent) = 0;
	bool   (*GetTraceState)(void* self) = 0;

	size_t tableSize;
	TraceTableElement first;

	inline TraceTableElement* begin()
	{
		return &first;
	}

	inline TraceTableElement* end()
	{
		return &first + tableSize;
	}

	inline size_t GetTotalSize()
	{
		return sizeof(TraceTable) - sizeof(first) + tableSize * sizeof(first);
	}

	inline TraceTableView* ToView()
	{
		return reinterpret_cast<TraceTableView*>(this);
	}

	inline static void Log()
	{
		CONSOLE_LOG() << s_allocator->TotalAllocatedBytes() << " bytes for all TraceTables\n";
	}

#define DefineHasClassMethod(method) 										\
	template <typename T>													\
	class Has_##method														\
	{																		\
		typedef char one;													\
		struct two { char x[2]; };											\
		template <typename C> static one test( decltype(&C::method) ) ;		\
		template <typename C> static two test(...); 						\
	public:																	\
		enum { value = sizeof(test<T>(0)) == sizeof(char) };				\
	};

	DefineHasClassMethod(Trace);
#undef DefineHasClassMethod

	template <typename T>
	static TraceTable* Get();
};

class __TraceTablesFinalizer
{
public:
	static __TraceTablesFinalizer __traceTablesFinalizer;

	__TraceTablesFinalizer()
	{
		TraceTable::s_allocator = NewMalloc<SimpleStackBasedAllocator>(128 * KB);
	}

	~__TraceTablesFinalizer()
	{
		DeleteMalloc(TraceTable::s_allocator);
	}
};

inline __TraceTablesFinalizer __TraceTablesFinalizer::__traceTablesFinalizer;

template <typename T>
class Traceable;

template <typename T>
class ManagedPointer;

template <typename T>
class ManagedPointer;

class Tracer
{
public:
	// to placeholder, same memory layout with TraceTable
#ifdef _TRACE_DEBUG
	TraceTableView* m_view;
	const char* m_className = 0;
#endif

	Dtor m_dtor = 0;

	size_t m_instanceSize;
	size_t (*m_GetDynamicArraySize)(void* parent) = 0;
	bool   (*m_GetTraceState)(void* self) = 0;

	size_t m_cId = 0;
	TraceTableElement m_table[TRACE_TABLE_MAX_SIZE];

	// allow max 1024 traces
	//void* m_offsets[KB] = {};

	void* m_current = 0;

	static Tracer s_tracer;

public:
	void Begin()
	{
		m_cId = 0;
	}

public:
	template <template <typename> class P, typename T>
	void TraceManagedPtr(P<T>& ptr);

	// careful
	void TraceRawPtr(void* ptr)
	{
		auto t = m_cId++;
		m_table[t].offset = (byte*)ptr - (byte*)m_current;
		m_table[t].traceTable = 0;
		m_table[t].count = 1;
	}

	// only trace on Ptr<T>
	template <typename T>
	void Trace(T& ptr);

	// only trace on Ptr<T>
	template <typename T>
	void TraceNoExpand(T& ptr);

	template<typename T, std::size_t N>
	void Trace(T (&arr)[N]);

	void BindDynamicArrayGetSize(size_t(*GetDynamicArraySize)(void*))
	{
		m_GetDynamicArraySize = GetDynamicArraySize;
	}

	void BindDestructor(void (*dtor)(void*))
	{
		m_dtor = dtor;
	}

	void BindGetTraceState(bool (*call)(void*))
	{
		m_GetTraceState = call;
	}

	template <typename T>
	void Mimic(void* p)
	{
		static_assert(std::is_base_of_v<Traceable<T>, T>);
		reinterpret_cast<T*>(p)->Trace(this);
	}

	template <typename T, bool TRACE = true>
	TraceTable* TraceClass(T* clazz)
	{
		m_current = clazz;

		m_cId = 0;
		m_instanceSize = sizeof(T);

		m_GetDynamicArraySize = 0;
		m_GetTraceState = 0;

		m_dtor = [](void* self)
		{
			((T*)self)->~T();
		};

#ifdef _TRACE_DEBUG
		m_className = typeid(T).name();
#endif

		if constexpr (TRACE) clazz->Trace(this);

		TraceTable* temp = reinterpret_cast<TraceTable*>(this);

#ifdef _TRACE_DEBUG
		temp->view = temp->ToView();
#endif // _DEBUG

		auto totalSize = temp->GetTotalSize();

		TraceTable::s_lock.lock();
		auto table = (TraceTable*)TraceTable::s_allocator->Allocate(totalSize);
		TraceTable::s_lock.unlock();

		memcpy(table, temp, totalSize);

#ifdef _TRACE_DEBUG
		table->view = table->ToView();
#endif // _DEBUG

		return table;
	}

};

inline Tracer Tracer::s_tracer;

struct TracerPool
{
	inline static Pool<sizeof(Tracer), 8> s_pool = { 1 };
	inline static spinlock s_lock;

	inline static Tracer* Get()
	{
		s_lock.lock();
		Tracer* tracer = (Tracer*)s_pool.Allocate();
		s_lock.unlock();
		return tracer;
	}

	inline static void Return(Tracer* tracer)
	{
		s_lock.lock();
		s_pool.Deallocate(tracer);
		s_lock.unlock();
	}
};

class AtomicTraceable
{
public:
	inline static TraceTable* s_traceTable = 0;

	byte m_data[SIZE_OF_MANAGED_PTR];

public:
	void Trace(Tracer* tracer)
	{
		tracer->TraceRawPtr(this);
	}

};

template <typename T>
class Traceable
{
private:
	TRACEABLE_FRIEND();

	inline static TraceTable* _GetTraceTable()
	{
		if (s_traceTable != 0)
		{
			return s_traceTable;
		}

		Tracer* tracer = TracerPool::Get();
		if constexpr (is_specialization<T, ManagedPointer>{})
		{
			if (AtomicTraceable::s_traceTable == 0)
			{
				AtomicTraceable::s_traceTable = tracer->TraceClass((AtomicTraceable*)0);
			}
			s_traceTable = AtomicTraceable::s_traceTable;
		}
		else
		{
			s_traceTable = tracer->TraceClass((T*)0);
		}
		TracerPool::Return(tracer);

		return s_traceTable;
	}

	inline static TraceTable* s_traceTable = _GetTraceTable();

public:
	inline static TraceTable* GetTraceTable()
	{
		return s_traceTable;
	}

};


template <typename T>
class TraceableNoExpand : public Traceable<T> {};


class TraceablePOD
{
private:
	TRACEABLE_FRIEND();

	struct POD
	{
		size_t sampleField;

		void Trace(Tracer* tracer)
		{
			// no destructor
			tracer->BindDestructor(0);
		}
	};

	inline static TraceTable* _GetTraceTable()
	{
		if (s_traceTable != 0)
		{
			return s_traceTable;
		}

		Tracer* tracer = TracerPool::Get();
		s_traceTable = tracer->TraceClass<POD>((POD*)0);
		TracerPool::Return(tracer);

		return s_traceTable;
	}

	inline static TraceTable* s_traceTable = _GetTraceTable();

public:
	inline static TraceTable* GetTraceTable()
	{
		return s_traceTable;
	}
};


template <typename T>
class TraceableNone
{
private:
	TRACEABLE_FRIEND();

	inline static TraceTable* _GetTraceTable()
	{
		if (s_traceTable != 0)
		{
			return s_traceTable;
		}

		Tracer* tracer = TracerPool::Get();
		s_traceTable = tracer->TraceClass<T, false>((T*)0);
		TracerPool::Return(tracer);

		return s_traceTable;
	}

	inline static TraceTable* s_traceTable = _GetTraceTable();

public:
	inline static TraceTable* GetTraceTable()
	{
		return s_traceTable;
	}

};


template<template <typename> class P, typename T>
inline void Tracer::TraceManagedPtr(P<T>& ptr)
{
	static_assert(std::is_base_of_v<ManagedPointer<T>, P<T>>, "Type check!");
	//static_assert(std::is_base_of_v<Traceable<T>, T>, "Type check!");

	auto t = m_cId++;
	m_table[t].offset = (byte*)&ptr - (byte*)m_current;
	m_table[t].traceTable = 0;//Traceable<T>::GetTraceTableIdx();
	m_table[t].count = 1;
}


template <typename T>
inline void Tracer::Trace(T& ptr)
{
	static_assert(std::is_base_of_v<Traceable<T>, T>, "Type check!");

	if constexpr (std::is_base_of_v<TraceableNoExpand<T>, T>)
	{
		TraceNoExpand(ptr);
	}
	else
	{
		ptr.Trace(this);
	}
}

template<typename T>
inline void Tracer::TraceNoExpand(T& ptr)
{
	static_assert(std::is_base_of_v<Traceable<T>, T>, "Type check!");

	auto t = m_cId++;
	m_table[t].offset = (byte*)&ptr - (byte*)m_current;
	m_table[t].traceTable = Traceable<T>::_GetTraceTable();
	m_table[t].count = 1;
}


template<typename T, std::size_t N>
inline void Tracer::Trace(T (&arr)[N])
{
	static_assert(std::is_base_of_v<Traceable<T>, T>, "Type check!");

	auto t = m_cId++;
	m_table[t].offset = (byte*)&arr - (byte*)m_current;
	m_table[t].traceTable = Traceable<T>::_GetTraceTable();
	m_table[t].count = N;
}

template <typename T>
inline static TraceTable* TraceTable::Get()
{
	if constexpr (std::is_base_of_v<Traceable<T>, T>)
	{
		static_assert(Has_Trace<T>::value, "Traceable must provides trace method \"void Trace(Tracer* tracer);\"");
		return Traceable<T>::GetTraceTable();
	}
	else if constexpr (std::is_pod_v<T> || std::is_base_of_v<TraceablePOD, T>)
	{
		return TraceablePOD::GetTraceTable();
	}
	else
	{
		return TraceableNone<T>::GetTraceTable();
	}
}

NAMESPACE_MEMORY_END