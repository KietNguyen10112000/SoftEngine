#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Structures/Managed/Array.h"

#include "Common/Base/MainComponent.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

class MainComponent;

class ModifiedRecorder
{
public:
	enum ACTION
	{
		NONE,
		ADD, 
		REMOVE
	};

	struct Record
	{
		Handle<MainComponent> comp;
		ID COMPONENT_ID;

		inline void Trace(Tracer* tracer)
		{
			tracer->Trace(comp);
		}
	};

	Array<Record> m_modifiedRecords;

public:
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_modifiedRecords);
	}

	inline void Record(const Handle<MainComponent>& comp, ID COMPONENT_ID)
	{
		if (comp->m_recorded)
		{
			return;
		}

		m_modifiedRecords.Push({ comp,COMPONENT_ID });
	}

	inline void Run()
	{
		for (auto& r : m_modifiedRecords)
		{
			ACTION action = ACTION::NONE;

			auto& comp = r.comp;

			auto srcObj = comp->m_committedObject;
			auto destObj = comp->m_object;

			auto srcScene = srcObj->GetScene();

			if (destObj == nullptr)
			{
				
			}

		}
	}

};

NAMESPACE_END