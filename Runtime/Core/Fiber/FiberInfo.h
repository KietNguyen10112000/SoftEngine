#pragma once

#include "Core/Thread/ThreadLimit.h"

class FiberInfo
{
public:
	// num external fibers using by engine
	constexpr static size_t FIBERS_COUNT = 64;

	// include primary fibers
	constexpr static size_t TOTAL_FIBERS = ThreadLimit::MAX_THREADS + FIBERS_COUNT;

	constexpr static size_t STACK_SIZE = 512 * 1024;

};