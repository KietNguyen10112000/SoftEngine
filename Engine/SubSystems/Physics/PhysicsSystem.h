#pragma once

#include "Core/TypeDef.h"

#include "Core/Pattern/Singleton.h"

NAMESPACE_BEGIN

class Scene;

class PhysicsSystem : public Singleton<PhysicsSystem>
{
public:
	void Process(Scene* scene);

};

NAMESPACE_END