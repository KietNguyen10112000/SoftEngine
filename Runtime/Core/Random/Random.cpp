#include "Random.h"

#include "Core/Time/Clock.h"

NAMESPACE_BEGIN

std::mt19937 Random::generators[ThreadLimit::MAX_THREADS];

void Random::Initialize(size_t* seeds)
{
	if (seeds)
	{
		for (size_t i = 0; i < ThreadLimit::MAX_THREADS; i++)
		{
			generators[i].seed(seeds[i]);
		}
	}
	else
	{
		for (size_t i = 0; i < ThreadLimit::MAX_THREADS; i++)
		{
			generators[i].seed(Clock::ns::now());
		}
	}
}

NAMESPACE_END