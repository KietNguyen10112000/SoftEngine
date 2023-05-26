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

template <class T, typename F>
size_t VTableIndex(F f)
{
#define DEFINE_INDEX(n) virtual size_t Get##n() { return n;}
#define DEFINE_INDEX_10(n)	\
	DEFINE_INDEX(n##0);		\
	DEFINE_INDEX(n##1);		\
	DEFINE_INDEX(n##2);		\
	DEFINE_INDEX(n##3);		\
	DEFINE_INDEX(n##4);		\
	DEFINE_INDEX(n##5);		\
	DEFINE_INDEX(n##6);		\
	DEFINE_INDEX(n##7);		\
	DEFINE_INDEX(n##8);		\
	DEFINE_INDEX(n##9);	
	

	struct VTableCounter
	{
		DEFINE_INDEX(0);
		DEFINE_INDEX(1);
		DEFINE_INDEX(2);
		DEFINE_INDEX(3);
		DEFINE_INDEX(4);
		DEFINE_INDEX(5);
		DEFINE_INDEX(6);
		DEFINE_INDEX(7);
		DEFINE_INDEX(8);
		DEFINE_INDEX(9);

		DEFINE_INDEX_10(1);
		DEFINE_INDEX_10(2);
		DEFINE_INDEX_10(3);
		DEFINE_INDEX_10(4);
		DEFINE_INDEX_10(5);
		DEFINE_INDEX_10(6);
		DEFINE_INDEX_10(7);
		DEFINE_INDEX_10(8);
		DEFINE_INDEX_10(9);
		// ... more ...
	} vt;

#undef DEFINE_INDEX_10
#undef DEFINE_INDEX

	T* t = reinterpret_cast<T*>(&vt);

	typedef size_t (T::* GetIndex)();
	GetIndex getIndex = (GetIndex)f;
	return (t->*getIndex)();
}

inline void* GetVTable(void* baseInstance)
{
	return (void*)*(size_t*)(baseInstance);
}

//template <auto FUNC, typename Base, typename Derived>
//inline bool IsOverridden(Base* baseInstance, Derived* derivedInstance)
//{
//	using vtable = size_t;
//
//	auto offset = VTableIndex<Base>(FUNC);
//
//	auto baseVtb = (vtable*)*(size_t*)(baseInstance);
//	auto derivedVtb = (vtable*)*(size_t*)(derivedInstance);
//
//	if (*(baseVtb + offset) != *(derivedVtb + offset))
//	{
//		return true;
//	}
//	
//	return false;
//}

template <auto FUNC, typename Base, typename Derived>
inline bool IsOverridden(void* baseVTable, Derived* derivedInstance)
{
	using vtable = size_t;

	auto offset = VTableIndex<Base>(FUNC);

	auto baseVtb = (vtable*)baseVTable;
	auto derivedVtb = (vtable*)*(size_t*)(derivedInstance);

	if (*(baseVtb + offset) != *(derivedVtb + offset))
	{
		return true;
	}

	return false;
}

NAMESPACE_END