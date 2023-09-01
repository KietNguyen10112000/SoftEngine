#pragma once

#include <stdint.h>
#include <type_traits>

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

//namespace memory
//{
//#define _REL_WITH_DEB_INFO

// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENV64
#else
#define ENV32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENV64
#else
#define ENV32
#endif
#endif

#define NAMESPACE_MEMORY_BEGIN NAMESPACE_BEGIN
#define NAMESPACE_MEMORY_END NAMESPACE_END

#define CONSOLE_LOG() std::cout << "[   LOG    ]\t"
#define CONSOLE_WARN() std::cout << "[   WARN   ]\t"

using byte = uint8_t;

constexpr static size_t SIZE_OF_MANAGED_PTR = 3 * sizeof(void*);

#ifdef ENV64

using bf_t = int64_t;

constexpr static size_t INTMAX_T_SIGN_BIT_MASK_1 = 0x8000'0000'0000'0000;
constexpr static size_t INTMAX_T_SIGN_BIT_MASK_0 = 0x7FFF'FFFF'FFFF'FFFF;

#else

using bf_t = int32_t;

constexpr static size_t INTMAX_T_SIGN_BIT_MASK_1 = 0x8000'0000;
constexpr static size_t INTMAX_T_SIGN_BIT_MASK_0 = 0x7FFF'FFFF;

#endif // ENV64

#if defined _DEBUG || defined _REL_WITH_DEB_INFO
#define DEBUG_CODE(code) code
#else
#define DEBUG_CODE(code)
#endif // _DEBUG


//using intmax_t = int64_t;


inline bool IsLeftMostBitEqual1(intmax_t v)
{
	return v & INTMAX_T_SIGN_BIT_MASK_1;
}

inline void SetLeftMostBit(intmax_t& v)
{
	v |= INTMAX_T_SIGN_BIT_MASK_1;
}

inline void ClearLeftMostBit(intmax_t& v)
{
	v &= INTMAX_T_SIGN_BIT_MASK_0;
}

inline intmax_t GetClearedLeftMostBit(intmax_t v)
{
	return v & INTMAX_T_SIGN_BIT_MASK_0;
}

template<typename T>
struct member_pointer_value;

template<typename Class, typename Value>
struct member_pointer_value<Value Class::*>
{
	using type = Value;
};

template<typename Class, typename Value, size_t N>
struct member_pointer_value<Value Class::* [N]>
{
	using type = Value;
};

template <class T, template <class...> class Template>
struct is_specialization : std::false_type {};

template <template <class...> class Template, class... Args>
struct is_specialization<Template<Args...>, Template> : std::true_type {};


class FreeBlock;

class AllocatedBlock
{
public:
	size_t m_totalSize = 0;

	union
	{
		AllocatedBlock* m_prev = 0;
		FreeBlock* m_prev_;
		void* opaque;
	};

	//size_t m_mem;

public:
	inline AllocatedBlock* NextBlock()
	{
		return (AllocatedBlock*)((byte*)this + m_totalSize);
	};

	inline AllocatedBlock* PrevBlock()
	{
		return m_prev;
	};

	inline bool IsAllocated()
	{
		bf_t bf = *(bf_t*)&m_totalSize;
		return bf > 2 || bf < -2;
	};

	inline auto TotalSize() { return m_totalSize; };

};


//}

NAMESPACE_END