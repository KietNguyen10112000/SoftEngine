#pragma once

#include "Math/Math.h"

class SceneObject;
class SceneSharedObject;
class Engine;

class Controller
{
public:
	inline virtual ~Controller() {};

	virtual void Update(Engine* engine) = 0;
	virtual void WriteTo(SceneObject* object) = 0;
	virtual void WriteToShared(SceneSharedObject* object) = 0;

};