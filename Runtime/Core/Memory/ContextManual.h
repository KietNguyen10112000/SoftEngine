#pragma once

#include "Context.h"

NAMESPACE_MEMORY_BEGIN

namespace gc
{

// provide manualy gc call
// it can be named "ManualContext" but for easy to find this class along with "Context" class, the name will be "ContextManual"
class ContextManual
{
public:
	std::vector<MarkState, STDAllocatorMalloc<MarkState>> m_stack;

	ContextManual();

	void Mark(byte MARK_VALUE);
	void CallDestructor(ManagedHandle* handle);

	// full system, stop the world GC with custom mark value
	void DoGC(byte MARK_VALUE, System* system, byte* RESET_MARK_VALUES, ManagedHeap** heap, size_t heapCount);

};

}
NAMESPACE_MEMORY_END