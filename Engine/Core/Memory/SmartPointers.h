#pragma once

#include "TypeDef.h"

#include <memory>

#include "Core/Structures/STD/STDAllocator.h"

#include "Memory.h"

NAMESPACE_MEMORY_BEGIN

template <typename T>
using UniquePtr = ::std::unique_ptr<T>;

template <typename T>
using SharedPtr = ::std::shared_ptr<T>;

template <typename T>
using WeakPtr = ::std::weak_ptr<T>;

template <typename T, typename... Args>
UniquePtr<T> MakeUnique(Args&&... args)
{
	return UniquePtr<T>(rheap::New<T>(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args)
{
	STDAllocator<T> alloc = {};
	return ::std::allocate_shared<T, STDAllocator<T>>(alloc, std::forward<Args>(args)...);
}

template <typename Derived, typename Base>
SharedPtr<Derived> SharedPtrStaticCast(const SharedPtr<Base>& ptr)
{
	return std::static_pointer_cast<Derived>(ptr);
}

NAMESPACE_MEMORY_END