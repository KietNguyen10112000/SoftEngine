#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class SubSystemInfo2D
{
public:
	/// 
	/// rendering
	/// physics
	/// scripting
	/// audio
	/// 
	constexpr static size_t INDEXED_SUBSYSTEMS_COUNT = 4;

	//constexpr static size_t SUBSYSTEMS_COUNT = 8;

	inline static size_t s_numAvailableIndexedSubSystemsCount = INDEXED_SUBSYSTEMS_COUNT;
	inline static size_t GetNumAvailabelIndexedSubSystemCount()
	{
		return  s_numAvailableIndexedSubSystemsCount;
	}

};

NAMESPACE_END