#pragma once

#include "Core/Structures/String.h"

class GTestLogger
{
public:
	class Color
	{
	public:
		static constexpr const char* RED = "\033[0;1;31m";
		static constexpr const char* GREEN = "\033[0;1;32m";
	};
	

public:
	inline static void Log(soft::String str, const char* color = Color::GREEN)
	{
		std::cout << color << "\033[1M[    LOG   ] " << str << "\033[0;0m" "\n";
	}

	inline static void Stream(soft::String str, const char* color = Color::GREEN)
	{
		std::cout << color << "\033[1M[    LOG   ] " << str << "\033[0;0m" "\n" "\033[F";
	}

};