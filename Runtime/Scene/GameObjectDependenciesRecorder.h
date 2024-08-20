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
	friend class GameObjectDependencies;

	struct RecordedObject
	{
		GameObject* obj;
		Scene* originalScene;
	};

	std::vector<RecordedObject> m_objects;
	std::vector<GameObject*> m_rootObjects;

	Scene* m_scene = nullptr;

private:
	void UnRecordAll(Scene* scene);

public:
	GameObjectDependenciesRecorder(Scene* scene);

	void Record(GameObject* obj);
	bool IsRecorded(GameObject* obj);

	/*inline auto& GetObjects()
	{
		return m_objects;
	}*/

	inline auto& GetRootObjects()
	{
		return m_rootObjects;
	}

};

NAMESPACE_END