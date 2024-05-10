#pragma once
#include <vector>

#include "Core/Memory/Memory.h"

#include "EditorContext.h"

using namespace soft;

namespace soft
{
	class GameObject;
	class Scene;
}

class EditorTab
{
public:
	ID m_id = 0;
	String m_name = "Unnamed";
	Scene* m_scene = nullptr;

public:
	virtual void OnObjectsAdded(std::vector<GameObject*>& objects) = 0;
	virtual void OnObjectsRemoved(std::vector<GameObject*>& objects) = 0;
	virtual void OnRenderGUI() = 0;
	virtual void OnRenderInGameDebugGraphics() = 0;

	inline auto GetScene()
	{
		return m_scene;
	}

};

