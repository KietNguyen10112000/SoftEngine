#include "Core/TypeDef.h"

#include "Core/Fiber/TypeDef.h"

#define NAMESPACE_PLATFORM_BEGIN NAMESPACE_BEGIN namespace platform {
#define NAMESPACE_PLATFORM_END NAMESPACE_END }

NAMESPACE_PLATFORM_BEGIN

FiberNativeHandle* ConvertThisThreadToFiber();
FiberNativeHandle* CreateFiber();
void DeleteFiber(FiberNativeHandle* handle);
void SwitchToFiber(FiberNativeHandle* handle);

NAMESPACE_PLATFORM_END