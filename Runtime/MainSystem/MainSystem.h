#pragma once

#include "Core/TypeDef.h"

#include "MainSystemInfo.h"

#include "Scene/Scene.h"

NAMESPACE_BEGIN

class GameObject;
class MainComponent;

class MainSystem
{
private:
	MAIN_SYSTEM_FRIEND_CLASSES();

protected:
	Scene* m_scene = nullptr;

	ConcurrentArrayList<Handle<void>>* m_handleKeeper = nullptr;
	
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

public:
	inline auto GetScene()
	{
		return m_scene;
	}

	template <typename T>
	inline ID KeepForFrame(const Handle<T>& handle)
	{
		return m_handleKeeper[m_scene->GetCurrentDeferBufferIdx()].Add(handle);
	}

};

NAMESPACE_END