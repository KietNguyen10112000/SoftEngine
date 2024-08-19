#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Memory/SmartPointers.h"
#include "Core/Pattern/Singleton.h"

#include "MainSystem/MainSystemInfo.h"

#include "GameObjectDependenciesRecorder.h"


NAMESPACE_BEGIN

class GameObjectDependenciesResolver
{
public:
	Scene* m_scene = nullptr;

	inline virtual ~GameObjectDependenciesResolver() {};

	inline virtual void Begin(GameObjectDependenciesRecorder* recorder, Scene* scene, GameObject* input) {};

	virtual void Resolve(GameObjectDependenciesRecorder* recorder, GameObject* input) = 0;

};

class GameObjectDependencies : public Singleton<GameObjectDependencies>
{
private:
	SharedPtr<GameObjectDependenciesResolver> m_solvers[MainSystemInfo::COUNT] = {};

public:
	GameObjectDependencies();

	void Collect(Scene* scene, GameObject* obj, GameObjectDependenciesRecorder* output);

};

NAMESPACE_END