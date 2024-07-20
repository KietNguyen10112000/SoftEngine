#pragma once

#include "TaskSystem/TaskSystem.h"

#include "PxPhysicsAPI.h"

NAMESPACE_BEGIN

class PhysXAllocator : public physx::PxAllocatorCallback
{
public:
	inline virtual void* allocate(size_t size, const char* typeName, const char* filename, int line) override
	{
#ifdef _DEBUG
		return size ? rheap::malloc_debug(size, typeName) : NULL;
#else
		return size ? rheap::malloc(size) : NULL;
#endif // _DEBUG
	}

	inline virtual void	deallocate(void* ptr) override
	{
		if (ptr)
			rheap::free(ptr);
	}

};

NAMESPACE_END