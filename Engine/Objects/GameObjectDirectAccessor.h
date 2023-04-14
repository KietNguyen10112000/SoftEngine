#pragma once

#include "Core/TypeDef.h"

#include "GameObject.h"

NAMESPACE_BEGIN

class GameObjectDirectAccessor
{
public:
	inline static ID GetObjectAABBQueryId(GameObject* obj)
	{
		return obj->m_aabbQueryId;
	}

	inline static void SetObjectAABBQueryId(GameObject* obj, ID id)
	{
		obj->m_aabbQueryId = id;
	}

public:
	inline static void BranchAdd(GameObject* obj)
	{
		obj->m_numBranch++;
	}

	inline static void BranchRemove(GameObject* obj)
	{
		obj->m_numBranch--;
	}

	inline static void BranchReset(GameObject* obj)
	{
		obj->m_numBranchCount.store(0, std::memory_order_relaxed);
	}

	// return true if current branch is the last branch merge to main branch
	// the last branch can perform merge process for this obj
	inline static bool BranchMerge(GameObject* obj)
	{
#ifdef _DEBUG
		auto ret = ++(obj->m_numBranchCount);
		assert(ret <= obj->m_numBranch.load(std::memory_order_relaxed));
		if (ret == obj->m_numBranch.load(std::memory_order_relaxed))
		{
			return true;
		}
		return false;
#else
		return (++obj->m_numBranchCount) == obj->m_numBranch.load(std::memory_order_relaxed);
#endif // _DEBUG
	}
};

NAMESPACE_END