#pragma once

#include <chrono>

#ifdef _WIN32
// just Sleep
#include <Windows.h>
#else
#include <unistd.h>
#define Sleep(t) usleep(t)
#endif // _WIN32


class FrameRateControl
{
public:
	/// 
	/// beginTime in millisec
	/// 
	inline static void Control(int64_t frameRate, int64_t beginTime)
	{
		auto current = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1'000'000;

		int64_t deltaTime = current - beginTime;

		int64_t frameTime = 1000 / frameRate;

		int64_t sleepTime = frameTime - deltaTime;

		if (sleepTime > 5)
		{
			Sleep(sleepTime - 5);
		}
	};

};