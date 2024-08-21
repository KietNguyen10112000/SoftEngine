#include "GameObjectDependenciesRecorder.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

GameObjectDependenciesRecorder::GameObjectDependenciesRecorder(Scene* scene) : m_scene(scene)
{
}

void GameObjectDependenciesRecorder::UnRecordAll(Scene* scene)
{
	for (auto& o : m_recordedObjects)
	{
		o->m_dependenciesRecordedValue = 0;
	}
}

void GameObjectDependenciesRecorder::FindCommonRoot(RecordedTree* pTree)
{
	auto& tree = *pTree;
	if (tree.descendants.size() == 1)
	{
		m_rootObjects.push_back(tree.descendants[0]);
		return;
	}

	struct Temp
	{
		GameObject* obj;
		int depth = 0;
	};

	auto FindDepth = [](GameObject* o) -> int
		{
			int depth = 0;
			while (o->Parent().Get())
			{
				depth++;
				o = o->Parent().Get();
			}
			return depth;
		};

	int minDepth = INT_MAX;
	std::vector<Temp> descendantDatas;
	for (auto& o : tree.descendants)
	{
		auto d = FindDepth(o);
		descendantDatas.push_back({ o, d });
		minDepth = std::min(minDepth, d);
	}

	if (minDepth == 0)
	{
		m_rootObjects.push_back(tree.root);
		return;
	}

	for (auto& data : descendantDatas)
	{
		auto offset = data.depth - minDepth;
		for (int i = 0; i < offset; i++)
		{
			data.obj = data.obj->Parent().Get();
		}
		assert(FindDepth(data.obj) == minDepth);
	}

	auto curDepth = minDepth;
	while (curDepth != 0)
	{
		// filter duplicated
		for (int i = 0; i < int(descendantDatas.size()); i++)
		{
			auto obj = descendantDatas[i].obj;
			if (obj->m_dependenciesRecordedValue == INT_MAX)
			{
				STD_VECTOR_ROLL_TO_FILL_BLANK_2(descendantDatas, i);
				i--;
				continue;
			}

			obj->m_dependenciesRecordedValue = INT_MAX;
		}

		assert(descendantDatas.size() != 0);
		
		if (descendantDatas.size() == 1)
		{
			auto& data = descendantDatas[0];
			m_rootObjects.push_back(data.obj);
			data.obj->m_dependenciesRecordedValue = 0;
			return;
		}

		for (auto& data : descendantDatas)
		{
			data.obj->m_dependenciesRecordedValue = 0;
			data.obj = data.obj->Parent().Get();
		}

		curDepth--;
	}

	assert(0 && "Unreachable");
}

void GameObjectDependenciesRecorder::FindCommonRoot()
{
	for (auto& tree : m_recordedTrees)
	{
		FindCommonRoot(&tree.second);
	}
}

void GameObjectDependenciesRecorder::Record(GameObject* obj)
{
	auto root = obj->m_root;
	assert(root->m_scene == nullptr || root->m_scene == m_scene || IsRecorded(root)); // Reaching this assertion means you are adding objects having dependency-game-objects which is in difference scenes
	assert(obj->m_scene == nullptr || obj->m_scene == m_scene || IsRecorded(obj));
	assert(obj->m_scene == root->m_scene);

	if (IsRecorded(obj))
	{
		return;
	}

	auto it = m_recordedTrees.find(root);
	if (it == m_recordedTrees.end())
	{
		it = m_recordedTrees.insert({ root, {root,root->m_scene} }).first;
	}

	auto& tree = it->second;

	tree.descendants.push_back(obj);
	m_recordedObjects.push_back(obj);
	obj->m_dependenciesRecordedValue = 1;

	auto curScene = m_scene;
	obj->PostTraversal(
		[this, curScene](GameObject* o) {
			assert(o->m_scene == nullptr || o->m_scene == curScene || IsRecorded(o)); // Same as above

			if (IsRecorded(o))
			{
				return;
			}

			m_recordedObjects.push_back(o);
			o->m_dependenciesRecordedValue = 1;
		}
	);

	//m_rootObjects.push_back(root);
}

bool GameObjectDependenciesRecorder::IsRecorded(GameObject* obj)
{
	return obj->m_dependenciesRecordedValue != 0;
}

NAMESPACE_END