#pragma once
#include <vector>

#include "Scene/GameObject.h"
#include "Common/Base/SerializableDB.h"

#include "Runtime/Runtime.h"

#include "Core/Thread/ReentrantLock.h"

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
	static EditorContext* s_instance;

	struct OxyzRenderConfig
	{
		float AxisXLength = 0;
		float AxisYLength = 0;
		float AxisZLength = 0;

		bool RenderOxzGrid = false;
		bool RenderOxzPlane = false;
		Vec2 OxzRangeStart = { -100,-100 };
		Vec2 OxzRangeEnd = { 100,100 };
		float OxzRangeStep = 10;
		float OxzGridThickness = 0.05f;
	};

	enum EVENT
	{
		MENU_ON_SAVE,
		MENU_ON_OPEN,

		COUNT
	};

	Array<Handle<EditorTab>> m_tabs;

	ID m_currentTabId = INVALID_ID;
	ID m_runTimeId = INVALID_ID;

	std::vector<SerializableDB::SerializableRecord*> m_components[MainSystemInfo::COUNT];

	ReentrantLock m_lock;
	bool m_padd[3];

	EditorTabFactory* m_tabFactory = nullptr;

	String m_savePath = "./Editor/";

	EventDispatcher<EditorContext, EVENT::COUNT, EVENT, ID> m_eventDispatcher = { this };
	GenericStorage m_genericStorage;

private:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_tabs);
		tracer->Trace(m_genericStorage);
	}

public:
	EditorContext(Scene* initScene);

private:
	void RenderMenuBar();
	void ReloadSerializableList();

	void RenderTabBar();
	void RenderTabCreationPopUp();

public:
	void OnObjectsAdded(std::vector<GameObject*>& objects, Scene* scene);
	void OnObjectsRemoved(std::vector<GameObject*>& objects, Scene* scene);
	void OnRenderGUI();

	void OnRenderInGameDebugGraphics();

	void OnFinalize();

	void RenderOxyz(OxyzRenderConfig& config);

	void RunTab(const Handle<EditorTab>& tab);
	void CloseTab(const Handle<EditorTab>& tab);

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
		return s_instance;
	}

	inline static EditorContext* Get()
	{
		return s_instance;
	}

	inline const String& GetSavePath()
	{
		return m_savePath;
	}

	inline auto* EventDispatcher()
	{
		return &m_eventDispatcher;
	}

	inline auto* GenericStorage()
	{
		return &m_genericStorage;
	}

	inline auto* GetTab(ID id)
	{
		return m_tabs[id].Get();
	}
};

