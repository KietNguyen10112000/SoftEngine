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
	size_t updateTurn = 0;

	inline void Update(size_t turn)
	{
		if (turn <= updateTurn) return;

		backBufferIndex = (backBufferIndex + 1) % DEFERRED_BUFFER_CONFIG::NUM_BUFFER;
		updateTurn = turn;
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
				state->Update(m_turn);
			}
		);
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
	T m_buffer[DEFERRED_BUFFER_CONFIG::NUM_BUFFER] = {};

	// current buffer state
	DeferredBufferState m_state;

public:
	class Accessor
	{
	protected:
		friend class DeferredBuffer<T>;

		DeferredBufferState* m_targetState;
		T* m_read;
		T* m_write;

	public:
		Accessor(DeferredBufferState* targetState, T* read, T* write) : m_targetState(targetState), m_read(read), m_write(write) {};

	public:
		template <auto field>
		inline const auto& Get() const
		{
			//using P = decltype(field);
			//using R = typename member_pointer_value<P>::type;
			return (m_read->*field);
		}

		template <auto field, typename T = decltype(field)>
		inline Accessor& Set(const T& v)
		{
			(m_write->*field) = v;

			if (m_read != m_write)
			{
				m_read = m_write;
				DeferredBufferTracker::Get()->Track(m_targetState);
			}

			return *this;
		}

		inline const T* Read() const
		{
			return m_read;
		}

		inline T* BeginWrite()
		{
			return m_write;
		}

		inline void EndWrite()
		{
			m_read = m_write;
		}
	};

public:
	inline T* Read()
	{
		return &m_buffer[m_state.backBufferIndex];
	}

	inline Accessor GetAccessor() const
	{
		auto writeId = (m_state.backBufferIndex + 1) % DEFERRED_BUFFER_CONFIG::NUM_BUFFER;
		auto read = &m_buffer[m_state.backBufferIndex];
		auto write = &m_buffer[writeId];
		return { (DeferredBufferState*)&m_state,(T*)read,(T*)write};
	}

};

NAMESPACE_MEMORY_END