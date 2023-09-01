#pragma once

#include "TypeDef.h"
#include "ManagedLocalScope.h"
#include "ManagedPointers.h"

NAMESPACE_MEMORY_BEGIN

struct TraceTableElement;
struct TraceTable;

namespace gc
{

constexpr static size_t MANAGED_PTR_SIZE = sizeof(ManagedPointer<byte>);
constexpr static size_t MANAGED_PTR_SIZE_DIV_VOID_PTR_SIZE = MANAGED_PTR_SIZE / sizeof(void*);


//inline constexpr void (*Mark)(byte**) = MarkTempl<true, false>;
//inline constexpr void (*MarkLocal)(byte**) = MarkTempl<false, true>;

//inline void Mark(byte** p)
//{
//	MarkTempl<true, false>(p);
//}
//
//inline void MarkLocal(byte** p)
//{
//	MarkTempl<false, true>(p);
//}
//
//inline void UnMarkLocal(byte** p)
//{
//	MarkTempl<false, true, 0>(p);
//}

struct MarkState
{
	TraceTableElement* it;
	//TraceTableElement* prev;

	//size_t itCount;
	//size_t itCurrentOffset;
	//size_t once;

	TraceTable* traceTable;
	byte* blockEnd;

	byte* ptr;
	byte** ppptr;

	size_t padding;
};


void Mark(byte** p);

}


NAMESPACE_MEMORY_END