#pragma once

#include "Core/TypeDef.h"
#include "Core/Thread/Spinlock.h"
#include "Core/Thread/ThreadID.h"
#include "Core/Thread/ThreadUtils.h"

#include "Array.h"

NAMESPACE_BEGIN

template <typename T, size_t N_SPACES = 4>
class ConcurrentList
{
private:
	Array<T> m_lists[N_SPACES] = {};
	Spinlock m_locks[N_SPACES] = {};

private:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_lists);
	}

	inline void LockAll()
	{
		for (size_t i = 0; i < N_SPACES; i++)
		{
			m_locks[i].lock();
		}
	}

	inline void UnlockAll()
	{
		for (size_t i = 0; i < N_SPACES; i++)
		{
			m_locks[i].unlock();
		}
	}

public:
	template <bool AVOID_THREAD_COLLIDE = false>
	void Add(const T& v)
	{
		size_t start = -1;

		if constexpr (AVOID_THREAD_COLLIDE)
		{
			start = ThreadID::Get() % N_SPACES;
		}

		auto chosenId = ThreadUtils::RingBufferLock<false>(start, m_locks, nullptr);
		m_lists[chosenId].Push(v);
		m_locks[chosenId].unlock();
	}

	void Clear()
	{
		for (size_t i = 0; i < N_SPACES; i++)
		{
			m_locks[i].lock();
			m_lists[i].clear();
			m_locks[i].unlock();
		}
	}

	template <typename Callback>
	void ForEach(Callback callback)
	{
		LockAll();

		for (auto& list : m_lists)
		{
			for (auto& v : list)
			{
				callback(v);
			}
		}

		UnlockAll();
	}

};

template <typename Container, size_t N_SPACES = 4>
class ConcurrentListC
{
private:
	Container m_lists[N_SPACES];

public:
	friend class Tracer;
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_lists);
	}

	inline void LockAll()
	{
		for (size_t i = 0; i < N_SPACES; i++)
		{
			m_lists[i].lock();
		}
	}

	inline void UnlockAll()
	{
		for (size_t i = 0; i < N_SPACES; i++)
		{
			m_lists[i].unlock();
		}
	}

public:
	//inline ConcurrentListC() {};

	template <typename T, bool AVOID_THREAD_COLLIDE = false>
	void Add(const T& v)
	{
		size_t start = -1;

		if constexpr (AVOID_THREAD_COLLIDE)
		{
			start = ThreadID::Get() % N_SPACES;
		}

		auto chosenId = ThreadUtils::RingBufferLock<false>(start, m_lists, nullptr);
		m_lists[chosenId].Add(v);
		m_lists[chosenId].unlock();
	}

	void Clear()
	{
		for (size_t i = 0; i < N_SPACES; i++)
		{
			m_lists[i].lock();
			m_lists[i].Clear();
			m_lists[i].unlock();
		}
	}

	template <typename Callback>
	void ForEachContainer(Callback callback)
	{
		LockAll();

		for (auto& list : m_lists)
		{
			callback(list);
		}

		UnlockAll();
	}

};

template <typename T>
class ConcurrentArrayList
{
public:
	struct RingIteration
	{
		size_t							id;
		size_t							size;
		T* begin;

		inline RingIteration& operator++()
		{
			id = (id + 1) % size;
			return *this;
		}

		inline RingIteration& operator+=(size_t n)
		{
			id = (id + n) % size;
			return *this;
		}

		inline bool operator==(const RingIteration& iter)
		{
			return id == iter.id;
		}

		inline bool operator!=(const RingIteration& iter)
		{
			return id != iter.id;
		}

		inline T& operator*()
		{
			return begin[id];
		}
	};

	struct ConsumeHead
	{
		T* begin;
		std::atomic<intmax_t>	size;

		ConsumeHead() {};
		ConsumeHead(const ConsumeHead& v)
		{
			*this = v;
		}

		inline bool TryTake(T& output)
		{
			if (size.load(std::memory_order_relaxed) < 0)
			{
				return false;
			}

			auto idx = --size;
			if (idx < 0)
			{
				return false;
			}

			output = begin[idx];
			return true;
		}

		void operator=(const ConsumeHead& v)
		{
			begin = v.begin;
			size = v.size.load(std::memory_order_relaxed);
		}
	};

public:
	Array<T>			m_buffer;
	std::atomic<size_t> m_size = { 0 };
	std::atomic<size_t> m_numWriter = { 0 };
	spinlock			m_growthLock;

private:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_buffer);
	}

	inline void _GrowthCapacity()
	{
		auto capacity = m_buffer.size();
		if (capacity == 0)
		{
			m_buffer.Resize(16);
		}
		else
		{
			// growth by 2
			auto newCapacity = 2 * capacity;
			m_buffer.Resize(newCapacity);
		}

	}

	inline void WaitForNoWriter()
	{
		while (m_numWriter.load(std::memory_order_relaxed) != 0)
		{
			std::this_thread::yield();
		}
	}

	inline void TryGrowthCapacity(size_t expectCapacity)
	{
		WaitForNoWriter();
		m_growthLock.lock();

		while (m_buffer.size() <= expectCapacity)
		{
			_GrowthCapacity();
		}

		m_growthLock.unlock();
	}

public:
	template <bool AVOID_THREAD_COLLIDE = false>
	inline ID Add(const T& v)
	{
		auto id = m_size++;

		if (id >= m_buffer.size())
		{
			TryGrowthCapacity(id);
		}

		++m_numWriter;
		m_buffer[id] = v;
		--m_numWriter;
		return id;
	}

	inline ID EmplaceBack()
	{
		auto id = m_size++;

		if (id >= m_buffer.size())
		{
			TryGrowthCapacity(id);
		}

		return id;
	}

	inline void Set(ID index, const T& v)
	{
		++m_numWriter;
		m_buffer[index] = v;
		--m_numWriter;
	}

	void Clear()
	{
		m_size = 0;
	}

	template <typename Callback>
	void ForEach(Callback callback)
	{
		auto _size = size();
		for (size_t i = 0; i < _size; i++)
		{
			auto& elm = m_buffer[i];
			callback(elm);
		}
	}

	inline size_t size() const
	{
		return m_size.load(std::memory_order_relaxed);
	}

	template <bool AVOID_THREAD_COLLIDE = false>
	inline T Take()
	{
		return m_buffer[--m_size];
	}

	// no thread-safe
	// output size must >= N_SPACES
	// output2 size = size() - output1Count
	inline void Split(size_t output1Count, T** output1, T** output2)
	{
		intmax_t _size = size();
		output1[0] = 0;
		output2[0] = 0;

		intmax_t remain = _size - output1Count;

		if (_size != 0)
		{
			auto end = _size >= output1Count ? output1Count : _size;
			output1[0] = &m_buffer[0];
			output1[1] = output1[0] + end;
			output1[2] = 0;
		}

		if (remain > 0)
		{
			output2[0] = output1[1];
			output2[1] = output2[0] + remain;
			output2[2] = 0;
		}
	}

	inline T* GetChunkAddrAtIndex(size_t id)
	{
		assert(id == 0);
		return m_buffer.data();
	}

	inline size_t GetChunkSizeAt(size_t id)
	{
		assert(id == 0);
		return m_buffer.size();
	}

	/*inline void Resize(size_t size)
	{
		m_size.exchange(size, std::memory_order_relaxed);
	}*/

	inline size_t GetChunks(T** output)
	{
		size_t i = 0;
		if (size() != 0)
		{
			output[i++] = &m_buffer[0];
			output[i++] = &m_buffer.back() + 1;
		}
		output[i] = 0;
		return i;
	}

	inline RingIteration RingBegin()
	{
		RingIteration ret;
		ret.id = 0;
		ret.size = m_size.load(std::memory_order_relaxed);
		ret.begin = m_buffer.data();
		return ret;
	}

	inline ConsumeHead GetComsumeHead()
	{
		ConsumeHead ret;
		ret.begin = m_buffer.data();
		ret.size = (intmax_t)m_size.load(std::memory_order_relaxed);
		return ret;
	}

	inline void UpdateFromConsumeHead(const ConsumeHead& head)
	{
		auto size = head.size.load(std::memory_order_relaxed);
		m_size = size < 0 ? 0 : size;
	}

	inline auto begin()
	{
		return m_buffer.data();
	}

	inline auto end()
	{
		return m_buffer.data() + m_size.load(std::memory_order_relaxed);
	}

	inline void ReserveNoSafe(size_t size)
	{
		m_buffer.reserve(size);
	}
};


NAMESPACE_END