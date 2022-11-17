#pragma once

#include <cstdint>
#include <cstdlib>
#include <memory>

template <typename T, typename idx_t = size_t>
class UnorderedDynamicArray
{
protected:
	using byte = uint8_t;

	struct Element
	{
		T v;
		union
		{
			Element* next;
			idx_t idxOfAllocated;
		};
	};

	struct iterator
	{
	private:
		friend class UnorderedDynamicArray;

		Element* m_buffer;
		idx_t* m_idx;

		iterator(Element* buffer, idx_t* idx) : m_buffer(buffer), m_idx(idx) {};

	public:
		T& operator*() const 
		{
			return m_buffer[*m_idx].v;
		}

		T* operator->() 
		{ 
			return &m_buffer[*m_idx].v;
		}

		// Prefix increment
		iterator& operator++() 
		{
			m_idx++;
			return *this;
		}

		iterator& operator++(int)
		{
			iterator temp = *this;
			m_idx++;
			return temp;
		}

		bool operator==(const iterator& a) 
		{
			return m_idx == a.m_idx;
		}

		bool operator!=(const iterator& a) 
		{
			return m_idx != a.m_idx;
		}
	};

	Element* m_buffer = 0;
	Element* m_free = 0;

	union
	{
		Element* m_bufferEnd = 0;
		idx_t* m_allocatedBegin;
	};
	
	idx_t* m_allocatedIt = 0;
	idx_t* m_allocatedEnd = 0;

public:
	UnorderedDynamicArray(size_t size)
	{
		m_buffer = (Element*)malloc(size * (sizeof(Element) + sizeof(idx_t)));
		m_bufferEnd = m_buffer + size;
		m_allocatedIt = m_allocatedBegin;
		m_allocatedEnd = m_allocatedBegin + size;
		m_free = m_buffer;
		InitFreeLinkedList(m_buffer, size);
	}

	~UnorderedDynamicArray()
	{
		free(m_buffer);
	}

protected:
	void InitFreeLinkedList(Element* buffer, size_t size)
	{
		for (size_t i = 0; i < size - 1; i++)
		{
			buffer[i].next = &buffer[i + 1];
		}
		buffer[size - 1].next = 0;
	}

public:
	// C++ range-base for loop
	inline iterator begin()
	{
		return { m_buffer, m_allocatedBegin };
	}

	inline iterator end()
	{
		return { m_buffer, m_allocatedIt };
	}

public:
	inline size_t Size()
	{
		return m_allocatedIt - m_allocatedBegin;
	}

	inline void Resize(size_t newSize)
	{
		auto size = Size();
		if (size >= newSize)
		{
			return;
		}

		auto newBuffer = (Element*)malloc(newSize * (sizeof(Element) + sizeof(idx_t)));
		auto newBufferEnd = newBuffer + newSize;
		//auto newAllocatedIt = (idx_t*)newBufferEnd;
		auto newAllocatedEnd = (idx_t*)newBufferEnd + newSize;

		InitFreeLinkedList(newBuffer, newSize);

		auto offset = newBuffer - m_buffer;

		auto newAllocatedIt = (idx_t*)newBufferEnd + (m_allocatedIt - m_allocatedBegin);

		auto it = m_free;
		while (it)
		{
			auto next = it->next;
			if (next) it->next += offset;
			it = next;
		}

		memcpy(newBuffer, m_buffer, size * sizeof(Element));
		memcpy(newBufferEnd, m_allocatedBegin, size * sizeof(idx_t));

		if (m_free)
		{
			m_free = m_free + offset;
		}
		else
		{
			m_free = newBuffer + size;
		}

		free(m_buffer);

		m_buffer = newBuffer;
		m_bufferEnd = newBufferEnd;
		m_allocatedIt = newAllocatedIt;
		m_allocatedEnd = newAllocatedEnd;
	}

	inline idx_t Add(const T& v)
	{
		if (m_free == 0)
		{
			Resize(Size() * 2);
		}

		Element* elm = m_free;
		m_free = m_free->next;

		idx_t idx = elm - m_buffer;
		
		*m_allocatedIt = idx;

		elm->v = v;
		elm->idxOfAllocated = (m_allocatedIt++) - m_allocatedBegin;

		return idx;
	}

	// remove what returned by Add()
	inline void Remove(idx_t idx)
	{
		auto elm = &m_buffer[idx];
		auto idxAllocated = elm->idxOfAllocated;

		elm->next = m_free;
		m_free = elm;
		
		if ((--m_allocatedIt) == m_allocatedBegin) return;

		// fill blank element
		auto backIdx = *m_allocatedIt;
		m_allocatedBegin[idxAllocated] = backIdx;
	}

	// access what returned by Add()
	inline T& operator[](idx_t idx) const
	{
		return m_buffer[idx].v;
	}


	inline T& GetAllocatedAt(idx_t allocatedIdx) const
	{
		return m_buffer[m_allocatedBegin[allocatedIdx]].v;
	}

};