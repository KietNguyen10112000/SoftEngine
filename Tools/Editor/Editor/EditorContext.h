#pragma once
#include <vector>

#include "Scene/GameObject.h"
#include "Common/Base/SerializableDB.h"

#include "Runtime/Runtime.h"

using namespace soft;

struct GameObjectEditorComponent
{
	ID id;
};

class EditorTab;
class EditorTabFactory;

class EditorContext
{
public:
	static ID s_id;

	Array<Handle<EditorTab>> m_tabs;

	ID m_currentTabId = INVALID_ID;

	std::vector<SerializableDB::SerializableRecord*> m_components[MainSystemInfo::COUNT];

	ID m_runningThreadId = INVALID_ID;
	spinlock m_lock;
	bool m_padd[3];

	EditorTabFactory* m_tabFactory = nullptr;

private:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_tabs);
	}

public:
	EditorContext(Scene* initScene);

private:
	void RenderMenuBar();
	void ReloadSerializableList();

	void RenderTabBar();
	void RenderTabCreationPopUp();

public:
	void OnObjectsAdded(std::vector<GameObject*>& objects);
	void OnObjectsRemoved(std::vector<GameObject*>& objects);
	void OnRenderGUI();

	void OnRenderInGameDebugGraphics();

public:
	inline auto& Lock()
	{
		return m_lock;
	}

	inline auto GetCurrentTab()
	{
		return m_tabs[m_currentTabId].Get();
	}

	inline static EditorContext* GetInstance()
	{
		return Runtime::Get()->GenericStorage()->Get<EditorContext>(s_id);
	}

};

