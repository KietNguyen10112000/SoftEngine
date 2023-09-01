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
			//return std::chrono::steady_clock::now().time_since_epoch().count();
		}

		// return nanoseconds since epoch
		inline static size_t Now()
		{
			return now();
		}
	};

	class ms
	{
	public:
		inline static size_t now()
		{
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()
			);
			return ms.count();
		}

		inline static size_t Now()
		{
			return now();
		}

	};
	
};
