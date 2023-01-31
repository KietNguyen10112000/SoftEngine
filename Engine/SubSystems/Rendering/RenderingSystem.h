#pragma once

#include "Core/TypeDef.h"

#include "Core/Pattern/Singleton.h"

NAMESPACE_BEGIN

class Scene;

class RenderingSystem : public Singleton<RenderingSystem>
{
public:
	void Process(Scene* scene);

};

NAMESPACE_END