#pragma once
#include "Core/Memory/ManagedPointers.h"
#include "Core/Memory/Trace.h"
#include "Core/Structures/TypeDef.h"

NAMESPACE_BEGIN

template <typename T, typename ArrayType>
class ArrayTraceableElement
{
public:
	friend class Tracer;
	class Element : public Traceable<Element>
	{
		TRACEABLE_FRIEND();

		//using ArrayType = typename Array;

		byte m_mem[sizeof(T)];

		void Trace(Tracer* tracer)
		{
			tracer->BindDynamicArrayGetSize(
				[](void* owner) -> size_t
				{
					auto v = ((ArrayType*)((byte*)owner));
					return v->size();
				}
			);

			// no destuctor call on this block
			tracer->BindDestructor(0);

			if constexpr (std::is_base_of_v<Traceable<T>, T>)
			{
				//T::Trace(tracer);
				tracer->Mimic<T>(this);
			}
		}
	};

};

template <typename T>
class ArrayPODElement
{
public:
	friend class Tracer;
	class Element
	{
		byte m_mem[sizeof(T)];
	};

};


// gc dynamic array
template <typename T>
class Array : Traceable<Array<T>>
{
public:
	friend class Tracer;

	using Element = typename std::conditional<
		std::is_base_of_v<Traceable<T>, T>,
		typename ArrayTraceableElement<T, Array<T>>::Element,
		typename ArrayPODElement<T>::Element
	>::type;

private:
	Handle<T> m_buffer = nullptr;
	size_t m_size = 0;
	size_t m_capacity = 0;

	_MANAGED_CONTAINER_THREAD_SAFE(spinlock m_lock);

//public:
//	~Array()
//	{
//		int x = 3;
//	}

private:
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_buffer);
	}

protected:
	inline void _Reserve(size_t n)
	{
		if (n <= m_capacity) return;

		// growth by 2
		auto newCapacity = std::max(n, 2 * m_capacity);

		Local<T> oldBuf = m_buffer;
		Local<T> newBuf = ReinterpretCast<T>(mheap::Allocate<Element>(newCapacity));

		if (Size() != 0)
		{
			::memcpy(newBuf.Get(), oldBuf.Get(), sizeof(T) * Size());
		}

		m_buffer = newBuf;
		m_capacity = newCapacity;
	}

	inline void _Resize(size_t newSize)
	{
		auto currentSize = Size();
		if (newSize == currentSize) return;

		_Reserve(newSize);

		Local<T> buf = m_buffer;

		if (newSize > currentSize)
		{
			for (size_t i = currentSize; i < newSize; i++)
			{
				new (buf.Get() + i) T();
			}
		}
		else
		{
			for (intmax_t i = currentSize - 1; i >= newSize; i--)
			{
				(buf.Get() - i)->~T();
			}
		}
		m_size = newSize;
	}

public:
	inline size_t Size() const
	{
		return m_size;
	}

	inline size_t Capacity() const
	{
		return m_capacity;
	}

	inline T* Data() const
	{
		return m_buffer.Get();
	}

	inline void Push(const T& v)
	{
		_MANAGED_CONTAINER_CHECK_THREAD_SAFE(m_lock);

		_Reserve(Size() + 1);

		auto& _back = *(m_buffer.Get() + m_size);
		_back = v;

		m_size = Size() + 1;
	}

	inline void Resize(size_t newSize)
	{
		_MANAGED_CONTAINER_CHECK_THREAD_SAFE(m_lock);
		_Resize(newSize);
	}

public:
	// STL-like
	inline size_t size() const
	{
		return m_size;
	}

	inline size_t capacity() const
	{
		return m_capacity;
	}

	inline T* data() const
	{
		return m_buffer.Get();
	}

	inline T& back() const
	{
		return *(m_buffer.Get() + m_size - 1);
	}

};

NAMESPACE_END