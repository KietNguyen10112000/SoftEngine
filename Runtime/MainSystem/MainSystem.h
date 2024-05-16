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

	friend class ModifiedRecorder;
	struct ModificationAction
	{
		enum TYPE
		{
			ADD,
			REMOVE,
			MOVED
		};

		MainComponent* comp;
		TYPE type;
	};

	std::vector<ModificationAction> m_modificationActions;
	bool m_isAddAtGlobal = false;
	bool m_isRmAtGlobal = false;
	bool m_padd[6];

protected:
	Scene* m_scene = nullptr;
	
public:
	MainSystem(Scene* scene) : m_scene(scene) {};
	virtual ~MainSystem() {};

protected:
	virtual void FlushAsyncTasks() = 0;

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

	template <typename T>
	inline void InitializeAsyncTaskRunnerForMainComponent(T& runners)
	{
		for (auto& r : runners)
		{
			r.Initialize(&m_scene->m_currentDeferBufferIdx, &m_scene->m_prevDeferBufferIdx);
		}
	}

public:
	inline auto GetScene()
	{
		return m_scene;
	}

};

NAMESPACE_END