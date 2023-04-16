#pragma once

#include "SubSystems/SubSystem.h"

#include "Objects/Scene/Scene.h"

NAMESPACE_BEGIN

class Scene;

class RenderingSystem : public SubSystem
{
private:
	std::Vector<GameObject*> m_cameraObjects;

	UniquePtr<SceneQuerySession> m_dynamicQuerySession;

public:
	RenderingSystem(Scene* scene);

public:
	virtual void PrevIteration(float dt) override;

	virtual void Iteration(float dt) override;

	virtual void PostIteration(float dt) override;

public:
	inline void AddCamera(GameObject* obj)
	{
		m_cameraObjects.push_back(obj);
	}

};

NAMESPACE_END