#pragma once

#include "SubSystems/SubSystem.h"

NAMESPACE_BEGIN

class Scene;

class RenderingSystem : public SubSystem
{
private:
	std::Vector<GameObject*> m_cameraObjects;

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