#pragma once
#include <vector>
#include <map>
#include <string>

#include "STDAllocator.h"

//NAMESPACE_BEGIN

namespace std
{
	using namespace soft;

	// dynamic array
	template <typename T>
	using Vector = ::std::vector<T, STDAllocator<T>>;

	// dynamic array
	template <typename T>
	using Array = ::std::vector<T, STDAllocator<T>>;

	// map
	template <typename Key, typename Value>
	using Map = ::std::map<Key, Value, ::std::less<Key>, STDAllocator<::std::pair<const Key, Value>>>;

	using String = std::basic_string<char, std::char_traits<char>, STDAllocator<char>>;
}


#define STD_VECTOR_ROLL_TO_FILL_BLANK(v, removeObjName, idName)	\
if (v.size() == 1)												\
{																\
	v.clear();													\
} else {														\
auto& blank = v[removeObjName->idName];							\
auto& back = v.back();											\
back->idName = removeObjName->idName;							\
blank = back;													\
v.pop_back(); }


#define STD_VECTOR_ROLL_TO_FILL_BLANK_2(v, idx)					\
if (v.size() == 1)												\
{																\
	v.clear();													\
} else {														\
auto& blank = v[idx];											\
auto& back = v.back();											\
blank = back;													\
v.pop_back(); }

//NAMESPACE_END