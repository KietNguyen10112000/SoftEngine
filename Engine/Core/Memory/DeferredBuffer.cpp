#include "DeferredBuffer.h"

#include "SmartPointers.h"

NAMESPACE_MEMORY_BEGIN

void DeferredBufferTracker::Initialize()
{
	s_instance = MakeUnique<DeferredBufferTracker>();
}

void DeferredBufferTracker::Finalize()
{
	s_instance.reset();
}

NAMESPACE_MEMORY_END