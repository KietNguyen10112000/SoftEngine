#pragma once

#include "TypeDef.h"

NAMESPACE_MEMORY_BEGIN

// new using malloc

template <typename T, typename ...Args>
T* NewMallocArray(size_t n, Args&&... args)
{
	T* objs = (T*)::malloc(sizeof(T) * n);

	for (size_t i = 0; i < n; i++)
	{
		new (objs + i) T(std::forward<Args>(args)...);
	}

	return objs;
}

template <typename T, typename ...Args>
T* NewMalloc(Args&&... args)
{
	return NewMallocArray<T>(1, std::forward<Args>(args)...);
}

// just for single elm
template <typename T>
void DeleteMalloc(T* p)
{
	auto it = p;
	it->~T();
	::free(p);
}


template <typename T>
class STDAllocatorMalloc
{
public:
	using value_type = T;

	template <class U>
	struct rebind
	{
		using other = STDAllocatorMalloc<U>;
	};

	STDAllocatorMalloc() = default;
	template <class U> constexpr STDAllocatorMalloc(const STDAllocatorMalloc <U>&) noexcept {}

	[[nodiscard]] T* allocate(std::size_t n)
	{
		return (T*)::malloc(sizeof(T) * n);
	}

	void deallocate(T* p, std::size_t n) noexcept
	{
		return ::free(p);
	}
};

NAMESPACE_MEMORY_END