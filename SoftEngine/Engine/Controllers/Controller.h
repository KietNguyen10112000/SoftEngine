#pragma once

#include "Math/Math.h"

class SceneQueriedNode;
class SceneQueryContext;
class Engine;

class Controller
{
public:
	inline virtual ~Controller() {};

	virtual bool Update(SceneQueryContext* context, 
		SceneQueriedNode* node, const Mat4x4& globalTransform, Engine* engine) = 0;

	virtual void CallMethod(SceneQueryContext* context, SceneQueriedNode* node, const Mat4x4& globalTransform,
		int methodId, void* args, int argc, Engine* engine) = 0;

};