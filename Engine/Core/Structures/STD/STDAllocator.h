#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"

NAMESPACE_BEGIN

template <typename T>
class STDAllocator
{
public:
	using value_type = T;

	template <class U>
	struct rebind
	{
		using other = STDAllocator<U>;
	};

	STDAllocator() = default;
	template <class U> constexpr STDAllocator(const STDAllocator <U>&) noexcept {}

	[[nodiscard]] T* allocate(std::size_t n)
	{
		return (T*)rheap::malloc(sizeof(T) * n);
	}

	void deallocate(T* p, std::size_t n) noexcept
	{
		return rheap::free(p);
	}
};

NAMESPACE_END