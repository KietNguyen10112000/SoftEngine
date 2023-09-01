#pragma once

// it just a queue, so using library for better performance
#define USING_CONCURRENT_QUEUE_LIBRARY

#ifdef USING_CONCURRENT_QUEUE_LIBRARY
#include "Libraries/moodycamel/concurrentqueue.h"

template <typename T>
using ConcurrentQueue = moodycamel::ConcurrentQueue<T>;

#endif // USING_CONCURRENT_QUEUE_LIBRARY
