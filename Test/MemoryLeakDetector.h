#pragma once

#include <gtest/gtest.h>

#ifdef WIN32 || WIN64
#include <crtdbg.h>

class MemoryLeakDetector
{
public:
    MemoryLeakDetector(size_t allowedLeakBytes = 0) : m_allowedLeakBytes(allowedLeakBytes)
    {
        _CrtMemCheckpoint(&memState_);
    }

    ~MemoryLeakDetector()
    {
        _CrtMemState stateNow, stateDiff;
        _CrtMemCheckpoint(&stateNow);
        int diffResult = _CrtMemDifference(&stateDiff, &memState_, &stateNow);
        if (diffResult)
            reportFailure(stateDiff.lSizes[1]);
    }
private:
    void reportFailure(size_t unfreedBytes)
    {
        if (unfreedBytes <= m_allowedLeakBytes) {
            return;
        }

        FAIL() << "Memory leak of " << unfreedBytes << " byte(s) detected.";
    }

    _CrtMemState memState_;
    size_t m_allowedLeakBytes;

};

#else
static_assert(0);
#endif