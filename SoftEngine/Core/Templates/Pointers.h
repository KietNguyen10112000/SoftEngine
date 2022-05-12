#pragma once

#include <memory>

//#if defined(_WINDOWS)
//#include <Window.h>
//#endif
//
//inline size_t GetMemSize(void* pointer)
//{
//#if defined(_WINDOWS)
//    return _msize(pointer);
//#else
//    static_assert(0, "re-implement");
//#endif
//};

#ifdef USE_STL_SMART_POINTERS

template <typename T>
using SharedPtr = std::shared_ptr<T>;

template <typename T>
using WeakPtr = std::weak_ptr<T>;

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

#endif // USE_STL_SMART_POINTERS
