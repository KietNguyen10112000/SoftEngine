#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Structures/Managed/Array.h"

#include "Common/Base/MainComponent.h"
#include "MainSystem/MainSystem.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

NAMESPACE_BEGIN

class MainComponent;

class ModifiedRecorder
{
public:
	/*enum ACTION
	{
		ADD, 
		REMOVE
	};*/

	struct Record
	{
		Handle<MainComponent> comp;
		ID COMPONENT_ID;

		inline void Trace(Tracer* tracer)
		{
			tracer->Trace(comp);
		}
	};

	struct WaitingComponent
	{
		MainSystem* system;
		MainComponent* comp;
	};

	ConcurrentArrayList<Handle<GameObject>> m_modifiedGameObjects;

	Array<Record> m_modifiedRecords;

	std::vector<MainSystem*> m_addingSystems;
	std::vector<MainSystem*> m_removingSystems;

	std::vector<WaitingComponent> m_removingComps;
	std::vector<WaitingComponent> m_addingComps;

public:
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_modifiedGameObjects);
		tracer->Trace(m_modifiedRecords);
	}

	inline void RecordComponent(const Handle<MainComponent>& comp, ID COMPONENT_ID)
	{
		if (comp->m_recorded)
		{
			return;
		}

		comp->m_recorded = true;
		m_modifiedRecords.Push({ comp,COMPONENT_ID });
	}

	inline void RecordGameObject(const Handle<GameObject>& obj, GameObject::ModifiedFlag::Flag cause)
	{
		obj->m_modifiedFlags |= cause;
		if (obj->m_recorded)
		{
			return;
		}

		obj->m_recorded = true;
		m_modifiedGameObjects.Add(obj);
	}

	inline void Clear()
	{
		m_modifiedGameObjects.Clear();
		m_modifiedRecords.clear();

		m_addingComps.clear();
		m_addingSystems.clear();

		m_removingComps.clear();
		m_removingSystems.clear();
	}

	inline void Commit()
	{
		for (auto& r : m_modifiedRecords)
		{
			auto& comp = r.comp;

			auto srcObj = comp->m_committedObject;
			auto destObj = comp->m_object;

			auto srcScene = srcObj ? srcObj->GetCommittedScene() : nullptr;
			auto destScene = destObj ? destObj->GetCurrentScene() : nullptr;

			auto* srcActions = srcScene ? &srcScene->GetMainSystem(r.COMPONENT_ID)->m_modificationActions : nullptr;
			auto* destActions = destScene ? &destScene->GetMainSystem(r.COMPONENT_ID)->m_modificationActions : nullptr;

			comp->m_committedObject = comp->m_object;
			comp->m_recorded = false;

			if (srcObj == nullptr && destObj == nullptr)
			{
				continue;
			}

			if (srcObj == nullptr && destObj != nullptr)
			{
				if (destActions)
				{
					destActions->push_back({ comp,MainSystem::ModificationAction::ADD });
				}
				continue;
			}

			if (srcObj != nullptr && destObj == nullptr)
			{
				if (srcActions)
				{
					srcActions->push_back({ comp,MainSystem::ModificationAction::REMOVE });
				}
				continue;
			}

			if (srcObj != nullptr && destObj != nullptr)
			{
				//if (srcObj == destObj)
				{
					if (srcScene == nullptr && destScene == nullptr)
					{
						continue;
					}

					if (srcScene != nullptr && destScene == nullptr)
					{
						srcActions->push_back({ comp,MainSystem::ModificationAction::REMOVE });
						continue;
					}

					if (srcScene == nullptr && destScene != nullptr)
					{
						destActions->push_back({ comp,MainSystem::ModificationAction::ADD });
						continue;
					}

					if (srcScene != nullptr && destScene != nullptr)
					{
						if (srcScene != destScene)
						{
							// components moved across scenes, need perform at global iteration

							auto src = srcScene->GetMainSystem(r.COMPONENT_ID);
							auto dest = destScene->GetMainSystem(r.COMPONENT_ID);

							if (src->m_isRmAtGlobal == false)
							{
								m_removingSystems.push_back(src);
								src->m_isRmAtGlobal = true;

								//src->BeginModification();
							}

							if (dest->m_isAddAtGlobal == false)
							{
								m_addingSystems.push_back(dest);
								dest->m_isAddAtGlobal = true;
							}

							//src->RemoveComponent(comp);
							//comp->OnComponentRemoved();
							//destScene->GetMainSystem(r.COMPONENT_ID)->AddComponent(comp);
							m_removingComps.push_back({ src,comp });
							m_addingComps.push_back({ dest,comp });
						}
					}
				}
			}
		}

		// commit changes
		for (auto& obj : m_modifiedGameObjects)
		{
			if (obj->GetCurrentScene() == obj->GetCommittedScene())
			{
				auto scene = obj->GetCommittedScene();
				if (scene)
				{
					if (obj->m_forceRefreshTransform || obj->m_committedGlobalTransform != obj->m_globalTransform)
					{
						for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
						{
							auto& comp = obj->m_mainComponents[i];
							if (comp && (obj->m_forceRefreshTransform || i != obj->m_componentIdModifyTransform))
							{
								auto sys = scene->GetMainSystem(i);
								if (sys)
								{
									sys->m_modificationActions.push_back({ comp,MainSystem::ModificationAction::MOVED });
								}
							}
						}
					}
				}
			}

			obj->Commit();
			obj->m_recorded = false;
		}

		// remove all from src
		{
			for (auto& sys : m_removingSystems)
			{
				sys->BeginModification();
			}

			for (auto& a : m_removingComps)
			{
				a.system->RemoveComponent(a.comp);
				a.comp->OnComponentRemoved();
			}

			for (auto& sys : m_removingSystems)
			{
				sys->EndModification();
				sys->m_isRmAtGlobal = false;
			}
		}
		
		// add all to dest
		{
			for (auto& sys : m_addingSystems)
			{
				sys->BeginModification();
			}

			for (auto& a : m_addingComps)
			{
				a.system->AddComponent(a.comp);
				a.comp->OnComponentAdded();
			}

			for (auto& sys : m_addingSystems)
			{
				sys->EndModification();
				sys->m_isAddAtGlobal = false;
			}
		}
		
	}
};

NAMESPACE_END