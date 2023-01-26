#include "Platform/Platform.h"

#include <Windows.h>

#include "Core/Fiber/FiberInfo.h"
#include "TaskSystem/TaskWorker.h"

NAMESPACE_PLATFORM_BEGIN

FiberNativeHandle* ConvertThisThreadToFiber()
{
	return (FiberNativeHandle*)ConvertThreadToFiber(0);
}

FiberNativeHandle* CreateFiber()
{
	return (FiberNativeHandle*)::CreateFiber(
		FiberInfo::STACK_SIZE,
		Thread::EntryPointOfFiber,
		0
	);
	//return nullptr;
}

void DeleteFiber(FiberNativeHandle* handle)
{
	if (handle) ::DeleteFiber((LPVOID)handle);
}

void SwitchToFiber(FiberNativeHandle* handle)
{
	::SwitchToFiber((LPVOID)handle);
}

NAMESPACE_PLATFORM_END