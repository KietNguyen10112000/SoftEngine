#pragma once

#include <stdint.h>

#define NAMESPACE_BEGIN namespace soft {
#define NAMESPACE_END }


#ifdef EXPORTS
#    define API __declspec(dllexport)
#else
#    define API __declspec(dllimport)
#endif

#ifdef STATIC_BUILD
#ifdef API
#undef API
#define API
#endif // API
#else
#endif // STATIC_BUILD

#ifdef _DEBUG
#define Throw(msg) throw msg ", file: " __FILE__ ", line: " _STRINGIZE(__LINE__)
#else
#define Throw(msg) throw msg
#endif // _DEBUG


constexpr static size_t KB = 1024;
constexpr static size_t MB = 1024 * 1024;
using byte = uint8_t;