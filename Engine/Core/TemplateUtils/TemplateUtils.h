#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

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


template <class T>
auto SeeBits(T func)
{
	union
	{
		T ptr;
		size_t i;
		void* p;
	};
	ptr = func;

	return i;
}

template <class T>
constexpr auto VtbOffsetOf(T func)
{
	return SeeBits(func);
}

template <typename T>
T* MakeDummyClass(void* mem)
{
	new (mem)T();
	return (T*)mem;
}

template <typename Base, typename Derived, auto FUNC>
inline bool IsOverridden(Base* baseInstance, Derived* derivedInstance)
{
	using vtable = size_t;

	auto offset = VtbOffsetOf(FUNC);

	auto baseVtb = (vtable*)*(size_t*)(baseInstance);
	auto derivedVtb = (vtable*)*(size_t*)(derivedInstance);

	if (*(baseVtb + offset) != *(derivedVtb + offset))
	{
		return true;
	}
	
	return false;
}

NAMESPACE_END