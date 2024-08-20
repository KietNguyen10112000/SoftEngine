#include "GameObjectDependenciesRecorder.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

GameObjectDependenciesRecorder::GameObjectDependenciesRecorder(Scene* scene) : m_scene(scene)
{
}

void GameObjectDependenciesRecorder::UnRecordAll(Scene* scene)
{
	for (auto& o : m_objects)
	{
		o.obj->m_scene = o.originalScene;
	}
}

void GameObjectDependenciesRecorder::Record(GameObject* obj)
{
	auto root = obj->m_root;
	assert(root->m_scene == nullptr || root->m_scene == m_scene || IsRecorded(root)); // Reaching this assertion means you are adding objects having dependency-game-objects which is in difference scenes

	if (IsRecorded(root))
	{
		return;
	}

	auto curScene = m_scene;
	root->PostTraversal(
		[this, curScene](GameObject* o) {
			assert(o->m_scene == nullptr || o->m_scene == curScene || IsRecorded(o)); // Same as above

			if (IsRecorded(o))
			{
				return;
			}

			m_objects.push_back({ o,o->m_scene });
			o->m_scene = (Scene*)INVALID_ID;
		}
	);

	m_rootObjects.push_back(root);
}

bool GameObjectDependenciesRecorder::IsRecorded(GameObject* obj)
{
	return obj->m_scene == (void*)INVALID_ID;
}

NAMESPACE_END