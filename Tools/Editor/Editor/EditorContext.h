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
		Vec4 AxisXColor = { 1,0,0,1 };
		float AxisYLength = 0;
		Vec4 AxisYColor = { 0,1,0,1 };
		float AxisZLength = 0;
		Vec4 AxisZColor = { 0,0,1,1 };

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

	enum DIALOG_RESULT
	{
		OK,
		CANCEL,
		CLOSE_CALL
	};

	enum class DIALOG_TYPE
	{
		OK,
		OK_CANCEL,
	};

	struct DialogDesc
	{
		String title = "";
		Vec2 size = { 0.5f,0.5f };
	};

	using DialogBodyCallback = void (*)(void*);

	// return true to close dialog
	using DialogResultCallback = bool (*)(DIALOG_RESULT, void*);

	struct DialogData
	{
	private:
		friend class EditorContext;
		String popUpId = "";

	public:
		DialogBodyCallback bodyCallback = nullptr;
		void* bodyUserPtr = nullptr;
		DialogResultCallback resultCallback = nullptr;
		void* resultUserPtr = nullptr;

		DialogDesc desc;

		DIALOG_TYPE type = DIALOG_TYPE::OK_CANCEL;

		DialogData(DialogBodyCallback a1, void* a2, DialogResultCallback a3, void* a4, const DialogDesc& a5)
			: bodyCallback(a1), bodyUserPtr(a2), resultCallback(a3), resultUserPtr(a4), desc(a5) {};
	};

	Array<Handle<EditorTab>> m_tabs;

	ID m_currentTabId = INVALID_ID;
	ID m_runTimeId = INVALID_ID;

	std::vector<SerializableDB::SerializableRecord*> m_components[MainSystemInfo::COUNT];

	ReentrantLock m_lock;
	bool m_padd[2];

	bool m_needCloseTabCreationPopUp = false;
	bool m_needOpennTabCreationPopUp = false;
	EditorTabFactory* m_tabFactory = nullptr;

	String m_savePath = "./Editor/";

	EventDispatcher<EditorContext, EVENT::COUNT, EVENT, ID> m_eventDispatcher = { this };
	GenericStorage m_genericStorage;

	std::vector<DialogData*> m_closeDialogs;
	std::vector<UniquePtr<DialogData>> m_dialogs;

	EditorTab* m_tabHolder = nullptr;

	String m_savingPath = "";

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

	void RenderDialogs();

	void CloseDialogImpl(DialogData* dialog);

	void DoSave(const String& path);

public:
	void OnObjectsAdded(std::vector<GameObject*>& objects, Scene* scene);
	void OnObjectsRemoved(std::vector<GameObject*>& objects, Scene* scene);
	void OnRenderGUI();

	void OnRenderInGameDebugGraphics();

	void OnFinalize();

	void RenderOxyz(OxyzRenderConfig& config);

	void RunTab(const Handle<EditorTab>& tab);
	void CloseTab(const Handle<EditorTab>& tab);

	bool IsVariableNameValid(const String& name);

	DialogData* OpenOkCancelDialog(const DialogDesc& desc, DialogBodyCallback bodyCallback, void* bodyUserPtr, DialogResultCallback resultCallback, void* resultUserPtr);
	void CloseDialog(DialogData* dialog);

	void OpenTabCreationPopUp();
	void CloseTabCreationPopUp();

	void PlaceHolderTab(EditorTab* tab);

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

