#pragma once

#include <stdint.h>

#define NAMESPACE_BEGIN namespace soft {
#define NAMESPACE_END }


#ifdef EXPORTS
#    define API __declspec(dllexport)
#else
#    define API __declspec(dllimport)
#endif

#ifdef EXPORTS
#    define API_VAR __declspec(dllexport)
#else
#    define API_VAR extern __declspec(dllimport)
#endif

#define IMPORT __declspec(dllimport)
#define EXPORT __declspec(dllexport)
#define EXTERN_C extern "C"

#ifdef STATIC_BUILD
#ifdef API
#undef API
#define API
#endif // API
#else
#endif // STATIC_BUILD

#define _STRINGIZEX(x)  #x
#define _STRINGIZE(x)   _STRINGIZEX(x)

#ifdef _DEBUG
#define Throw(msg) throw msg ", file: " __FILE__ ", line: " _STRINGIZE(__LINE__)
#else
#define Throw(msg) throw msg
#endif // _DEBUG


constexpr static size_t KB = 1024;
constexpr static size_t MB = 1024 * 1024;
using byte = uint8_t;
using ID = size_t;
constexpr static ID INVALID_ID = -1;

#define ATOMIC_EXCHANGE_ONCE(atomicVar, onceValue)							\
{																			\
	if (atomicVar.load(std::memory_order_relaxed) == onceValue) return;		\
	if (atomicVar.exchange(onceValue) == onceValue) return;					\
}

#define ATOMIC_EXCHANGE_ONCE_CALLBACK(atomicVar, onceValue, onceCallback)						\
{																								\
	if (atomicVar.load(std::memory_order_relaxed) == onceValue) { onceCallback; return; }		\
	if (atomicVar.exchange(onceValue) == onceValue) return;										\
}
