#pragma once
#include <chrono>

class Clock
{
public:
	class ns
	{
	public:
		// return nanoseconds since epoch
		inline static size_t now()
		{
			return std::chrono::high_resolution_clock::now().time_since_epoch().count();
		}

		// return nanoseconds since epoch
		inline static size_t Now()
		{
			return now();
		}
	};
	
};
