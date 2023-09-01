#pragma once

#include "TypeDef.h"

#include "Core/Structures/Raw/ConcurrentList.h"
#include "Core/Pattern/Singleton.h"

NAMESPACE_MEMORY_BEGIN

class DEFERRED_BUFFER_CONFIG
{
public:
	constexpr static size_t NUM_BUFFER = 2;

};

struct DeferredBufferState
{
	size_t backBufferIndex = 0;
	size_t updateSign = 0;

	// up to date read and write head
	void* readHead;
	void* writeHead;

#ifdef _DEBUG
	bool callTracked = false;
	Spinlock lock;
	bool padd[2];
#endif // _DEBUG


	inline void Update()
	{
		backBufferIndex = (backBufferIndex + 1) % DEFERRED_BUFFER_CONFIG::NUM_BUFFER;

#ifdef _DEBUG
		assert(callTracked == true);
		callTracked = false;
#endif // _DEBUG
	}
};

class API DeferredBufferTracker : public Singleton<DeferredBufferTracker>
{
public:
	constexpr static size_t N_SPACE = 16;
	raw::ConcurrentList<DeferredBufferState*, N_SPACE> m_buffers;
	size_t m_turn = 0;

public:
	static void Initialize();
	static void Finalize();

	inline void UpdateAllThenClear()
	{
		m_turn++;
		m_buffers.ForEach([=](DeferredBufferState* state)
			{
				state->Update();
			}
		);
		m_buffers.Clear();
	}

	inline void UpdateCustomBegin()
	{
		m_turn++;
	}

	inline void UpdateCustomEnd()
	{
		m_buffers.Clear();
	}

	inline void Track(DeferredBufferState* state)
	{
		m_buffers.Add(state);
	}

	inline void Reset()
	{
		UpdateAllThenClear();
		m_turn = 0;
	}

};

template <typename T>
class DeferredBuffer
{
protected:
	// current buffer state
	DeferredBufferState m_state;

	T m_buffer[DEFERRED_BUFFER_CONFIG::NUM_BUFFER] = {};

//public:
//	class Accessor
//	{
//	protected:
//		friend class DeferredBuffer<T>;
//
//		DeferredBufferState* m_targetState;
//		T* m_read;
//		T* m_write;
//
//	public:
//		Accessor(DeferredBufferState* targetState, T* read, T* write) : m_targetState(targetState), m_read(read), m_write(write) {};
//
//	public:
//		template <auto field>
//		inline const auto& Get() const
//		{
//			//using P = decltype(field);
//			//using R = typename member_pointer_value<P>::type;
//			return (m_read->*field);
//		}
//
//		template <auto field, typename T = decltype(field)>
//		inline Accessor& Set(const T& v)
//		{
//			(m_write->*field) = v;
//
//			if (m_read != m_write)
//			{
//				m_read = m_write;
//				DeferredBufferTracker::Get()->Track(m_targetState);
//			}
//
//			return *this;
//		}
//
//		inline const T* Read() const
//		{
//			return m_read;
//		}
//
//		inline T* BeginWrite()
//		{
//			return m_write;
//		}
//
//		inline void EndWrite()
//		{
//			m_read = m_write;
//		}
//	};

public:
	DeferredBuffer()
	{
		m_state.readHead = &m_buffer[m_state.backBufferIndex];
		m_state.writeHead = &m_buffer[(m_state.backBufferIndex + 1) % DEFERRED_BUFFER_CONFIG::NUM_BUFFER];
	}

public:
	inline void Init(const T& v)
	{
		for (auto& buf : m_buffer)
		{
			buf = v;
		}
	}

public:
	inline T* GetReadHead()
	{
		return &m_buffer[m_state.backBufferIndex];
	}

	inline T* GetWriteHead()
	{
		assert(GetReadHead() != (T*)m_state.writeHead);
		return (T*)m_state.writeHead;
	}

	inline T* GetUpToDateReadHead()
	{
		return (T*)m_state.readHead;
	}

	inline void UpdateReadWriteHead(size_t updateSign)
	{
#ifdef _DEBUG
		assert(m_state.lock.try_lock() == true);
#endif // _DEBUG

		if (m_state.updateSign == updateSign)
		{
#ifdef _DEBUG
			m_state.lock.unlock();
#endif // _DEBUG
			return;
		}

		m_state.updateSign = updateSign;

		m_state.readHead = m_state.writeHead;
		m_state.writeHead = &m_buffer[(m_state.backBufferIndex + 2) % DEFERRED_BUFFER_CONFIG::NUM_BUFFER];

#ifdef _DEBUG
		assert(m_state.callTracked == false);
		m_state.callTracked = true;
#endif // _DEBUG

		DeferredBufferTracker::Get()->Track(&m_state);

#ifdef _DEBUG
		m_state.lock.unlock();
#endif // _DEBUG
	}

};

NAMESPACE_MEMORY_END