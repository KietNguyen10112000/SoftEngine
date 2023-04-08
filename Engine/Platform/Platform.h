#pragma once

#include "Core/TypeDef.h"

#include "Core/Fiber/TypeDef.h"

#include "Core/Structures/String.h"

#define NAMESPACE_PLATFORM_BEGIN NAMESPACE_BEGIN namespace platform {
#define NAMESPACE_PLATFORM_END NAMESPACE_END }

NAMESPACE_BEGIN
class Input;
NAMESPACE_END


NAMESPACE_PLATFORM_BEGIN

using WindowNative = void;

FiberNativeHandle* ConvertThisThreadToFiber();
FiberNativeHandle* CreateFiber();
void DeleteFiber(FiberNativeHandle* handle);
void SwitchToFiber(FiberNativeHandle* handle);

#ifdef CreateWindow
#undef CreateWindow
#endif // CreateWindow

WindowNative* CreateWindow(::soft::Input* input, int x, int y, int width, int height, const char* title);
void DeleteWindow(WindowNative* window);

// return true if platform ask me quit
bool ProcessPlatformMsg(WindowNative* window);
void* GetWindowNativeHandle(WindowNative* window);


String GetExecutablePath();

NAMESPACE_PLATFORM_END