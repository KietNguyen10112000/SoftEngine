#pragma once

#include "SubSystems2D/SubSystem2D.h"

#include "Objects2D/Scene2D/Scene2D.h"

NAMESPACE_BEGIN

class Scene2D;
class Camera2D;

class RenderingSystem2D : public SubSystem2D
{
private:
	std::Vector<GameObject2D*> m_cameraObjects;

	Scene2DQuerySession* m_querySession;

public:
	RenderingSystem2D(Scene2D* scene);
	~RenderingSystem2D();

public:
	virtual void PrevIteration(float dt) override;
	virtual void Iteration(float dt) override;
	virtual void PostIteration(float dt) override;
	virtual void AddSubSystemComponent(SubSystemComponent2D* comp) override;
	virtual void RemoveSubSystemComponent(SubSystemComponent2D* comp) override;

public:
	void AddCamera(Camera2D* cam);

};

NAMESPACE_END