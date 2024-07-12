#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"

#include <vector>

NAMESPACE_BEGIN

class GameObject;
class Scene;

class GameObjectDependenciesRecorder
{
private:
	friend class Scene;
	std::vector<GameObject*> m_objects;
	std::vector<GameObject*> m_rootObjects;

private:
	void UnRecordAll();

public:
	GameObjectDependenciesRecorder(Scene* scene);

	void Record(GameObject* obj);
	bool IsRecorded(GameObject* obj);

};

class GameObjectDependenciesResolver
{
public:
	inline virtual ~GameObjectDependenciesResolver() {};

	virtual void Resolve(GameObjectDependenciesRecorder* recorder, GameObject* input) = 0;

};

NAMESPACE_END