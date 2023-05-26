#pragma once

#include "Objects2D/Scene2D/Scene2D.h"

#include "Components2D/Physics/Physics2D.h"

#include "SubSystems2D/Script/ScriptSystem2D.h"

NAMESPACE_BEGIN

// interface for script accessing to physics system
class PhysicsInterface
{
public:
	ScriptSystem2D* m_scriptSystem = nullptr;

	PhysicsInterface() {};

	PhysicsInterface(ScriptSystem2D* scriptSystem) : m_scriptSystem(scriptSystem)
	{
	}

public:
	// don't delete result, just use it
	// sortMask: Ray2DQueryInfo::SORK_MASK
	Ray2DQueryInfo* RayQuery(const Vec2& begin, const Vec2& end, size_t sortMask = 0)
	{
		return m_scriptSystem->RayQuery(begin, end, sortMask);
	}

};

NAMESPACE_END