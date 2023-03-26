#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class GameObject;

struct SceneQuerySession
{
	GameObject** begin = nullptr;
	GameObject** end = nullptr;

	virtual void Clear() = 0;
	virtual ~SceneQuerySession() {};
};

NAMESPACE_END