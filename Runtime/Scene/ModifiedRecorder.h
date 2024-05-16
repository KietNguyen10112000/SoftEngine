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

	struct AddingComponent
	{
		MainSystem* system;
		MainComponent* comp;
	};

	Array<Handle<GameObject>> m_modifiedGameObjects;

	Array<Record> m_modifiedRecords;

	std::vector<MainSystem*> m_addingSystems;
	std::vector<MainSystem*> m_removingSystems;

	std::vector<AddingComponent> m_addingComps;

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

	inline void RecordGameObject(const Handle<GameObject>& obj)
	{
		if (obj->m_recorded)
		{
			return;
		}

		obj->m_recorded = true;
		m_modifiedGameObjects.Push(obj);
	}

	inline void Clear()
	{
		m_modifiedGameObjects.clear();
		m_modifiedRecords.clear();

		m_addingComps.clear();
		m_removingSystems.clear();

		m_addingComps.clear();
	}

	inline void Commit()
	{
		for (auto& obj : m_modifiedGameObjects)
		{
			if (obj->GetCurrentScene() == obj->GetCommittedScene())
			{
				auto scene = obj->GetCommittedScene();
				if (scene)
				{
					if (obj->m_committedGlobalTransform != obj->m_globalTransform)
					{
						ID compId = 0;
						for (auto& comp : obj->m_mainComponents)
						{
							if (comp)
							{
								scene->GetMainSystem(compId)->m_modificationActions.push_back({ comp,MainSystem::ModificationAction::MOVED });
							}
							compId++;
						}
					}
				}
			}

			obj->Commit();
			obj->m_recorded = false;
		}

		for (auto& r : m_modifiedRecords)
		{
			auto& comp = r.comp;

			auto srcObj = comp->m_committedObject;
			auto destObj = comp->m_object;

			auto srcScene = srcObj->GetCommittedScene();
			auto destScene = destObj->GetCurrentScene();

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

								src->BeginModification();
							}

							if (dest->m_isAddAtGlobal == false)
							{
								m_addingSystems.push_back(dest);
								dest->m_isAddAtGlobal = true;
							}

							src->RemoveComponent(comp);
							//destScene->GetMainSystem(r.COMPONENT_ID)->AddComponent(comp);
							m_addingComps.push_back({ dest,comp });
						}
					}
				}
			}
		}

		for (auto& sys : m_removingSystems)
		{
			sys->EndModification();
			sys->m_isRmAtGlobal = false;
		}

		for (auto& sys : m_addingSystems)
		{
			sys->BeginModification();
		}

		for (auto& a : m_addingComps)
		{
			a.system->AddComponent(a.comp);
		}

		for (auto& sys : m_addingSystems)
		{
			sys->EndModification();
			sys->m_isAddAtGlobal = false;
		}
	}
};

NAMESPACE_END