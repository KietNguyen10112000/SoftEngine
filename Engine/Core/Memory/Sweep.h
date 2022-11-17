#pragma once

#include "TypeDef.h"
#include "ManagedPage.h"

NAMESPACE_MEMORY_BEGIN


namespace gc
{

struct SweepResult
{
	size_t totalReleasedBlocks = 0;
	size_t totalLocalBlocks = 0;
};

template <bool IS_LOCAL_SCOPE>
inline SweepResult SweepTempl(ManagedPage* page)
{
	SweepResult ret = {};

	static thread_local std::vector<void*> s_releasesList;

	page->ForEachAllocatedBlocks([&](ManagedHandle* handle) 
		{
			if (handle->marked)
			{
				handle->marked = 0;
				return;
			}

			ret.totalReleasedBlocks++;
			s_releasesList.push_back(handle);
		}
	);

	/*std::vector<AllocatedBlock*> temp;

	for (auto& v : s_releasesList)
	{
		temp.push_back((AllocatedBlock*)((ManagedHandle*)v - 2));
	}*/

	for (auto& v : s_releasesList)
	{
		ManagedHandle* handle = (ManagedHandle*)v;
		auto traceTable = TraceTable::Get(handle->traceTableIdx);
		++handle;
		//traceTable->dtor(++handle);
		page->Deallocate(handle);
	}

	s_releasesList.clear();

	return ret;
}


inline SweepResult Sweep(ManagedPage* page)
{
	return SweepTempl<false>(page);
}


}


NAMESPACE_MEMORY_END