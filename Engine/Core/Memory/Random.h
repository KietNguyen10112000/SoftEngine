#pragma once

#include <random>

class Random
{
public:
    inline static std::random_device s_dev;
    inline static std::mt19937_64 s_rand;
    inline static std::uniform_int_distribution<std::mt19937_64::result_type> s_uniformDist;

public:
    inline static void Reset()
    {
        s_rand = std::mt19937_64(s_dev());
    };

    inline static void Initialize()
    {
        Reset();
    };

    inline static void UnInitialize()
    {

    };

    inline static int32_t Int32(int32_t min, int32_t max)
    {
        return min + s_uniformDist(s_rand) % (max + 1 - min);
    };

    inline static int64_t Int64(int64_t min, int64_t max)
    {
        return min + s_uniformDist(s_rand) % (max + 1 - min);
    };

    inline static float Float(float min, float max)
    {
        auto range0_1 = (float)s_uniformDist(s_rand) / (float)std::numeric_limits<std::mt19937_64::result_type>::max();
        return range0_1 * (max - min) + min;
    };

    inline static double Double(double min, double max)
    {
        auto range0_1 = (double)s_uniformDist(s_rand) / (double)std::numeric_limits<std::mt19937_64::result_type>::max();
        return range0_1 * (max - min) + min;
    };

};