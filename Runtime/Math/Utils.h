#pragma once

#include <cmath>

namespace math
{

template <typename T>
inline T clamp(T v, T min, T max)
{
	return std::max(min, std::min(v, max));
}

}