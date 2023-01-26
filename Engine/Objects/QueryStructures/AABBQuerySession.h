#pragma once

#include "Core/TypeDef.h"

#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

class AABBQuerySession
{
public:
	std::Vector<void*> m_result;

public:
	inline void ClearPrevQueryResult()
	{
		m_result.clear();
	}

	inline auto& Result() const
	{
		return m_result;
	}

};

NAMESPACE_END