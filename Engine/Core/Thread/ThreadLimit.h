#pragma once

#include "Core/TypeDef.h"

class ThreadLimit
{
public:
	// max threads
	// n (num of hardware thread - 2) by engine and (32 - n) by user (create using script)
	constexpr static size_t MAX_THREADS = 32;

};