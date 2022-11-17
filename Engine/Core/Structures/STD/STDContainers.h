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
	using Array = ::std::vector<T, STDAllocator<T>>;

	// map
	template <typename Key, typename Value>
	using Map = ::std::map<Key, Value, ::std::less<Key>, STDAllocator<::std::pair<const Key, Value>>>;

	using String = std::basic_string<char, std::char_traits<char>, STDAllocator<char>>;
}

//NAMESPACE_END