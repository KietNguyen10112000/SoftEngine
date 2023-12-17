#pragma once
#include <vector>

#include "Scene/GameObject.h"
#include "Common/Base/SerializableDB.h"

using namespace soft;

struct GameObjectEditorComponent
{
	ID id;
};

class EditorContext
{
public:
	constexpr static size_t NAME_INPUT_MAX_LEN = 2048;

	Scene* m_scene;

	std::vector<GameObject*> m_objects;

	Handle<GameObject> m_inspectingObject;
	Handle<ClassMetadata> m_inspectingObjectData;

	std::vector<bool> m_inspectPropertiesIsOpenStack;
	std::vector<bool> m_inspectPropertiesIsRawInspectStack;
	std::vector<size_t> m_inspectInlinePropertiesCountStack;

	size_t m_selectionIdx = -1;

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

	std::vector<SerializableDB::SerializableRecord*> m_components[MainSystemInfo::COUNT];

	bool m_needReloadInspectingObject = false;
	struct CreateComponentContext
	{
		ID selectedComponentId = INVALID_ID;
		ID selectedIdx = INVALID_ID;
	} m_createComponentContext;

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
	void RenderMenuBar();

	void ShowCreateComponentPopup();
	void ShowCreateGameObjectPopup();

	void ReloadSerializableList();

public:
	EditorContext();

	void OnObjectsAdded(std::vector<GameObject*>& objects);
	void OnObjectsRemoved(std::vector<GameObject*>& objects);
	void OnRenderGUI();

	void OnRenderInGameDebugGraphics();

public:
	inline auto& Lock()
	{
		return m_lock;
	}

public:
	void Inspect(ClassMetadata* metaData);

};

