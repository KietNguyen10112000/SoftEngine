#pragma once

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