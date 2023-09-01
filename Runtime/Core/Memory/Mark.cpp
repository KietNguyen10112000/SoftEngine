//#include "Mark.h"
//
//#include <vector>
//
//#include "ManagedHandle.h"
//#include "Trace.h"
//
//NAMESPACE_MEMORY_BEGIN
//
//namespace gc
//{
//
//inline void PushNextTracePtrsToStack(std::vector<MarkState>& stack, byte* ptr)
//{
//	auto handle = (ManagedHandle*)ptr - 1;
//
//	auto block = (AllocatedBlock*)handle - 1;
//
//	// get data of state.it, state.end
//	TraceTable* traceTable = TraceTable::Get(handle->traceTableIdx);
//	stack.push_back(
//		{ 
//			traceTable->begin(), 0, 
//			traceTable->begin()->count, 0, 
//			traceTable, (byte*)block + block->TotalSize(),
//			ptr, 0
//		}
//	);
//}
//
//template <bool MARK_MARK, int MARK_LOCAL, byte LOCAL_VALUE = 1>
//inline void MarkTempl(byte** p)
//{
//	static thread_local std::vector<MarkState> s_stack;
//
//	//if constexpr (MARK_MARK || MARK_LOCAL == 2)
//	//{
//		s_stack.push_back({ 0, 0, 0, 0, 0, 0, *p, 0 });
//	//}
//
//	//if constexpr (MARK_LOCAL == 1)
//	//{
//	//	// p->local == 1 (prev atomic exchange)
//	//	PushNextTracePtrsToStack(s_stack, *p);
//	//}
//
//	while (!s_stack.empty())
//	{
//		MarkState& state = s_stack.back();
//
//		auto ptr = state.ptr;
//
//		if (state.it == 0)
//		{
//			auto handle = (ManagedHandle*)ptr - 1;
//			auto block = (AllocatedBlock*)handle - 1;
//
//			if (handle->marked == 1)
//			{
//				s_stack.pop_back();
//				continue;
//			}
//			handle->marked = 1;
//
//			// get data of state.it, state.end
//			TraceTable* traceTable = state.traceTable == 0 ? TraceTable::Get(handle->traceTableIdx) : state.traceTable;
//			state.it = traceTable->begin();
//			//state.end = traceTable->end();
//			state.prev = state.it - 1;
//
//			//state.instanceSize = traceTable->instanceSize;
//			state.traceTable = traceTable;
//
//			if (traceTable->tableSize != 0)
//			{
//				if (traceTable->GetDynamicArraySize)
//				{
//					if (s_stack.size() == 1)
//					{
//						state.blockEnd = 0;
//					}
//					else
//					{
//						auto size = traceTable->GetDynamicArraySize(state.ppptr);
//						state.blockEnd = (byte*)block + size * traceTable->instanceSize;
//					}
//				}
//				else
//				{
//					state.blockEnd = (byte*)block + block->TotalSize();
//				}
//			}
//			else
//			{
//				//state.blockEnd = 0;
//				s_stack.pop_back();
//				continue;
//			}
//
//		}
//		
//		if (state.it != state.traceTable->end())
//		{
//			/*if (state.it != state.prev)
//			{
//				state.itCount = state.it->count;
//				state.prev = state.it;
//			}*/
//
//			if (state.it->traceTableIdx == -1)
//			{
//				auto ppptr = (byte***)(ptr + state.it->GetOffset());
//				auto nextManagedPtr = *ppptr;
//				if (nextManagedPtr)
//				{
//					s_stack.push_back({ 0, 0, 0, 0, 0, 0, *nextManagedPtr, ppptr });
//				}
//			}
//			else if (state.itCurrentOffset != state.itCount)
//			{
//				auto ppptr = (byte***)(ptr + state.it->GetOffset());
//				s_stack.push_back({ 0, 0, 0, 0, TraceTable::Get(state.it->traceTableIdx), 0, (byte*)ppptr, ppptr});
//				state.itCurrentOffset++;
//				continue;
//			}
//			
//			state.it++;
//			state.itCurrentOffset = 0;
//			state.itCount = state.it->count;
//			continue;
//		}
//
//		state.ptr += state.traceTable->instanceSize;
//		if (state.ptr + state.traceTable->instanceSize <= state.blockEnd)
//		{
//			state.it = state.traceTable->begin();
//			continue;
//		}
//		
//		s_stack.pop_back();
//	}
//}
//
//void Mark(byte** p)
//{
//	MarkTempl<true, 0>(p);
//}
//
//}
//
//NAMESPACE_MEMORY_END