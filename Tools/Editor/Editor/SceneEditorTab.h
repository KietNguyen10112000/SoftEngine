#pragma once
#include <vector>

#include "EditorTab.h"

#include "Scene/GameObject.h"
#include "Common/Base/SerializableDB.h"

using namespace soft;

class SceneEditorTab : public EditorTab
{
public:
	struct GameObjectEditorComponent
	{
		ID id;
	};

	constexpr static size_t NAME_INPUT_MAX_LEN = 2048;

	std::vector<GameObject*> m_objects;

	Handle<GameObject> m_inspectingObject;
	Handle<ClassMetadata> m_inspectingObjectData;

	std::vector<bool> m_inspectPropertiesIsOpenStack;
	std::vector<bool> m_inspectPropertiesIsRawInspectStack;
	std::vector<size_t> m_inspectInlinePropertiesCountStack;

	//GameObjectEditorComponent
	ID m_selectionId = INVALID_ID;

	bool m_pinInspectPanel = true;
	bool m_pinHierarchyPanel = true;
	GameObject* m_dragingObject = nullptr;

	bool m_openInputNamePopup = false;
	GameObject* m_renameObject = nullptr;
	char m_nameInputTxt[NAME_INPUT_MAX_LEN] = {};

	char m_searchNameInputTxt[NAME_INPUT_MAX_LEN] = {};
	size_t m_searchNameIdx = -1;

	MainComponent* m_removeComp = nullptr;

	spinlock m_lock;

	bool m_needReloadInspectingObject = false;
	struct CreateComponentContext
	{
		ID selectedComponentId = INVALID_ID;
		ID selectedIdx = INVALID_ID;
	} m_createComponentContext;

	ID m_scriptsHotReloadListenerIdBegin = INVALID_ID;
	ID m_scriptsHotReloadListenerIdEnd = INVALID_ID;

	ID m_onSaveListenerId = INVALID_ID;

private:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_inspectingObject);
		tracer->Trace(m_inspectingObjectData);
	}

	void OnObjectSelected(GameObject* obj);

	void RenderHierarchyPanelOf(GameObject* obj);
	void RenderHierarchyPanel();
	void RenderInspectorPanel();

	void ShowCreateComponentPopup();
	void ShowCreateGameObjectPopup();

public:
	SceneEditorTab();

	void OnObjectsAdded(std::vector<GameObject*>& objects) override;
	void OnObjectsRemoved(std::vector<GameObject*>& objects) override;
	void OnRenderGUI() override;

	void OnRenderInGameDebugGraphics() override;
	
	void OnShow() override;
	void OnHide() override;
	void OnOpen() override;
	void OnClose() override;

public:
	inline auto& Lock()
	{
		return m_lock;
	}

public:
	void Inspect(ClassMetadata* metaData);

};

