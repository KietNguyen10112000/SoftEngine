#pragma once

#include "Core/TypeDef.h"

#include "MainSystemInfo.h"

NAMESPACE_BEGIN

class GameObject;
class MainComponent;

class MainSystem
{
private:
	MAIN_SYSTEM_FRIEND_CLASSES();

protected:
	Scene* m_scene = nullptr;
	
public:
	MainSystem(Scene* scene) : m_scene(scene) {};
	virtual ~MainSystem() {};

protected:
	virtual void BeginModification() = 0;

	// direct implementation
	virtual void AddComponent(MainComponent* comp) = 0;
	virtual void RemoveComponent(MainComponent* comp) = 0;
	virtual void OnObjectTransformChanged(MainComponent* comp) = 0;

	virtual void EndModification() = 0;

	virtual void PrevIteration() = 0;
	virtual void Iteration(float dt) = 0;
	virtual void PostIteration() = 0;

	inline virtual void Finalize() {};

};

NAMESPACE_END