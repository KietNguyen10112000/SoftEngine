#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"

#include <vector>
#include <map>

NAMESPACE_BEGIN

class GameObject;
class Scene;

class GameObjectDependenciesRecorder
{
private:
	//friend class Scene;
	friend class GameObjectDependencies;

	struct RecordedTree
	{
		GameObject* root;
		Scene* originalScene;
		std::vector<GameObject*> descendants;

		inline RecordedTree(GameObject* root, Scene* originalScene) : root(root), originalScene(originalScene) {}
	};

	std::map<GameObject*, RecordedTree> m_recordedTrees;
	std::vector<GameObject*> m_recordedObjects;
	std::vector<GameObject*> m_rootObjects;

	Scene* m_scene = nullptr;

public:
	GameObjectDependenciesRecorder(Scene* scene);

private:
	void UnRecordAll(Scene* scene);
	void FindCommonRoot(RecordedTree* tree);
	void FindCommonRoot();

public:
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