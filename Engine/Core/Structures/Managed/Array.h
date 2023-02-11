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

		byte m_mem[sizeof(T)] = {};

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

#define _MANAGED_ARRAY_CHECK_BOUND(index) assert(index >= 0 && index < m_size)

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
		Local<T> newBuf = ReinterpretCast<T>(mheap::NewArray<Element>(newCapacity));

		assert(newBuf.IsNull() == false);

		if (Size() != 0)
		{
			::memcpy(newBuf.Get(), oldBuf.Get(), sizeof(T) * Size());
			/*for (size_t i = 0; i < Size(); i++)
			{
				newBuf[i] = oldBuf[i];
			}*/
		}

		m_buffer = newBuf;
		m_capacity = newCapacity;
	}

	inline void _Resize(size_t newSize)
	{
		auto currentSize = Size();
		if (newSize <= currentSize)
		{
			m_size = newSize;
			return;
		}

		_Reserve(newSize);

		Local<T> buf = m_buffer;

		m_size = newSize;
		if (newSize > currentSize)
		{
			//for (size_t i = currentSize; i < newSize; i++)
			//{
			//	new (buf.Get() + i) T();
			//}

			mheap::CallConstructor(buf.Get() + currentSize, newSize - currentSize);
		}
		else
		{
			//for (intmax_t i = currentSize - 1; i >= (intmax_t)newSize; i--)
			//{
			//	(buf.Get() + i)->~T();
			//}
		}
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

		auto size = Size();

		_Reserve(size + 1);

		auto& _back = *(m_buffer.Get() + size);
		_back = v;

		m_size = size + 1;
	}

	inline T Pop()
	{
		_MANAGED_CONTAINER_CHECK_THREAD_SAFE(m_lock);

		T ret = back();
		Resize(Size() - 1);
		return ret;
	}

	inline void Resize(size_t newSize)
	{
		_MANAGED_CONTAINER_CHECK_THREAD_SAFE(m_lock);
		_Resize(newSize);
	}

	inline void Clear()
	{
		::memset(m_buffer.Get(), 0, sizeof(T) * m_size);
		m_size = 0;
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

	inline T* begin() const
	{
		return m_buffer.Get();
	}

	inline T* end() const
	{
		return m_buffer.Get() + m_size;
	}

	inline void clear()
	{
		Clear();
	}

public:
	inline T& operator[](size_t i) const
	{
		_MANAGED_ARRAY_CHECK_BOUND(i);
		return *(m_buffer.Get() + i);
	}

#undef _MANAGED_ARRAY_CHECK_BOUND

};

NAMESPACE_END