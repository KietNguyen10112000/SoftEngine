#pragma once

#include <random>

NAMESPACE_BEGIN

// procedure random generator
class ProcRandom
{
private:
	std::mt19937 generator;

public:
	inline void Seed(size_t seed)
	{
		generator.seed((uint32_t)seed);
	}

public:
	inline int32_t RangeInt32(int32_t min, int32_t max)
	{
		std::uniform_int_distribution<int32_t> distribution(min, max);
		return distribution(generator);
	}

	inline int64_t RangeInt64(int64_t min, int64_t max)
	{
		std::uniform_int_distribution<int64_t> distribution(min, max);
		return distribution(generator);
	}

	inline float RangeFloat(float min, float max)
	{
		std::uniform_real_distribution<float> distribution(min, max);
		return distribution(generator);
	}

	inline double RangeDouble(double min, double max)
	{
		std::uniform_real_distribution<double> distribution(min, max);
		return distribution(generator);
	}

};

NAMESPACE_END