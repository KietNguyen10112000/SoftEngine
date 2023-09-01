#pragma once

#include "Core/TypeDef.h"

#ifdef _DEBUG
#define _MANAGED_CONTAINER_THREAD_SAFE(x) x
#define _MANAGED_CONTAINER_CHECK_THREAD_SAFE(lock)  \
{ if (lock.try_lock() == false) { throw "_MANAGED_CONTAINER_CHECK_THREAD_SAFE failed"; }; lock.unlock(); }
#else
#define _MANAGED_CONTAINER_THREAD_SAFE(x)
#define _MANAGED_CONTAINER_CHECK_THREAD_SAFE(x)
#endif // _DEBUG