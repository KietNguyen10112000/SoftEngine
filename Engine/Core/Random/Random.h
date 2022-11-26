#pragma once
#include "Core/TypeDef.h"
#include "Core/Thread/Thread.h"
#include "Core/Thread/ThreadLimit.h"

#include <random>

NAMESPACE_BEGIN

// thread-safe random
class API Random
{
private:
	static std::mt19937 generators[ThreadLimit::MAX_THREADS];

public:
	static void Initialize(size_t* seeds = 0);

public:
	inline static int32_t RangeInt32(int32_t min, int32_t max)
	{
		std::uniform_int_distribution<int32_t> distribution(min, max);
		return distribution(generators[Thread::GetID()]);
	}

	inline static int64_t RangeInt64(int64_t min, int64_t max)
	{
		std::uniform_int_distribution<int64_t> distribution(min, max);
		return distribution(generators[Thread::GetID()]);
	}

	inline static float RangeFloat(float min, float max)
	{
		std::uniform_real_distribution<float> distribution(min, max);
		return distribution(generators[Thread::GetID()]);
	}

	inline static double RangeDouble(double min, double max)
	{
		std::uniform_real_distribution<double> distribution(min, max);
		return distribution(generators[Thread::GetID()]);
	}

};

NAMESPACE_END