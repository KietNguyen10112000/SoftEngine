#pragma once

#include "Core/Memory/Memory.h"
#include "Config.h"

NAMESPACE_BEGIN

class DeferredBufferControlBlock final
{
private:
	friend class Scene;

	size_t m_prevUpdateIteration;
	std::atomic<size_t> m_lastUpdateIteration;

	uint32_t m_writeIdx;
	const uint32_t m_N;
	const size_t m_sizeOfEach;

#ifdef _DEBUG
	size_t m_isWriting = 0;
#endif // _DEBUG

	void* m_read;
	void* m_write;

	inline void Update(size_t iteration)
	{
		assert(m_lastUpdateIteration.load(std::memory_order_relaxed) == iteration);
		m_prevUpdateIteration = iteration;

		m_writeIdx = (m_writeIdx + 1) % m_N;

		auto buffers = Buffers();
		m_read = m_write;
		m_write = buffers + m_sizeOfEach * m_writeIdx;
	}

	inline byte* Buffers()
	{
		return (byte*)(this + 1);
	}

};

///
/// this buffer will work if it has a scene
/// usage:
///		scene->BeginWrite(buffer);
///		// ... process data from buffer.Write()
///		scene->EndWrite(buffer);
/// 
template <typename T, uint32_t N = Config::NUM_DEFER_BUFFER>
class DeferredBuffer final
{
private:
	friend class Scene;

	size_t m_prevUpdateIteration = 0;
	std::atomic<size_t> m_lastUpdateIteration = { 0 };

	uint32_t m_writeIdx;
	const uint32_t m_N = N;
	const size_t m_sizeOfEach = sizeof(T);

#ifdef _DEBUG
	size_t m_isWriting = 0;
#endif // _DEBUG


	T* m_read = &m_buffers[(N - 1) % N];
	T* m_write = &m_buffers[0];

	T m_buffers[N] = {};

private:
	inline void CopyOnWrite()
	{
		*m_write = *m_read;
	}

public:
	inline void Initialize(const T& v)
	{
		for (auto& buf : m_buffers)
		{
			buf = v;
		}

		m_writeIdx = 0;
		m_read = &m_buffers[(N - 1) % N];
		m_write = &m_buffers[0];
	}

	inline const T* Read()
	{
		return m_read;
	}

	inline T* Write()
	{
#ifdef _DEBUG
		assert(m_isWriting != 0);
#endif // _DEBUG
		return m_write;
	}

	inline const T* UpToDateRead()
	{
		if (m_lastUpdateIteration.load(std::memory_order_relaxed) == m_prevUpdateIteration)
		{
			return Read();
		}

		return Write();
	}
};

NAMESPACE_END