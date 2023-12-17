#pragma once

#include "Core/Pattern/Singleton.h"
#include "Core/Structures/String.h"

#include "MainSystem/MainSystem.h"

class EditorContext;

using namespace soft;

class ComponentInspector : public Singleton<ComponentInspector>
{
public:
	using ComponentInspectorFunc = void(*)(EditorContext*, Serializable*, ClassMetadata*);

	std::map<String, ComponentInspectorFunc> m_map;

public:
	ComponentInspector();

public:
	static void InspectAnimatorSkeletalArray(EditorContext* ctx, Serializable* comp, ClassMetadata* meta);

public:
	inline bool Inspect(EditorContext* ctx, Serializable* comp, ClassMetadata* meta)
	{
		auto it = m_map.find(comp->GetClassName());
		if (it != m_map.end())
		{
			it->second(ctx, comp, meta);
			return true;
		}

		return false;
	}

};

