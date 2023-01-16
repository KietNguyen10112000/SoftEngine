#pragma once

#include "Core/TypeDef.h"

#include "Core/Pattern/Singleton.h"

NAMESPACE_BEGIN

class PhysicsSystem : public Singleton<PhysicsSystem>
{
public:
	void Iteration();

};

NAMESPACE_END