#include "GameObjectDependenciesResolver.h"

#include "MainSystem/Physics/PhysicsSystem.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

GameObjectDependencies::GameObjectDependencies()
{
	m_solvers[MainSystemInfo::PHYSICS_ID] = PhysicsSystem::GetDependenciesResolver();
}

void GameObjectDependencies::Collect(Scene* scene, GameObject* obj, GameObjectDependenciesRecorder* output)
{
	GameObjectDependenciesRecorder& recorder = *output;

	//GameObjectDependenciesResolver* resolvers[MainSystemInfo::COUNT] = {};
	for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
	{
		if (m_solvers[i])
		{
			m_solvers[i]->m_scene = scene;
			m_solvers[i]->Begin(&recorder, scene, obj);
		}
	}

	recorder.Record(obj);
	for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
	{
		auto& resolver = m_solvers[i];
		if (resolver)
		{
			for (size_t j = 0; j < recorder.m_objects.size(); j++)
			{
				auto o = recorder.m_objects[j].obj;
				if (o->m_mainComponents[i])
				{
					resolver->Resolve(&recorder, o);
				}
			}
		}
	}
	recorder.UnRecordAll(scene);
}

NAMESPACE_END