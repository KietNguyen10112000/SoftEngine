#pragma once
#include <vector>

#include "EditorTab.h"

#include "Scene/GameObject.h"
#include "Common/Base/SerializableDB.h"

using namespace soft;

class SceneEditorTab : public EditorTab
{
public:
	friend class AnimatorInspector;

	struct GameObjectEditorComponent
	{
		ID id = INVALID_ID;
		bool hotReloadFromFile = false;
		bool expandAll = false;
	};

	struct SceneEditorComponentData
	{
		bool lastOpen = false;
	};

	struct LoadedObjectFromFileData
	{
		String filePath;
		size_t loadedLastModifiedTime = 0;
	};

	constexpr static size_t NAME_INPUT_MAX_LEN = 2048;

	std::vector<GameObject*> m_objects;

	Handle<GameObject> m_inspectingObject;
	Handle<ClassMetadata> m_inspectingObjectData;

	std::vector<bool> m_inspectPropertiesIsOpenStack;
	std::vector<bool> m_inspectPropertiesIsRawInspectStack;
	std::vector<size_t> m_inspectInlinePropertiesCountStack;

	//GameObjectEditorComponent
	//ID m_selectionId = INVALID_ID;

	bool m_pinInspectPanel = true;
	bool m_pinHierarchyPanel = true;
	GameObject* m_dragingObject = nullptr;
	GameObject* m_willbeSelectedObject = nullptr;

	bool m_openInputNamePopup = false;
	GameObject* m_renameObject = nullptr;
	char m_nameInputTxt[NAME_INPUT_MAX_LEN] = {};
	String m_loadObjectFileName = "";

	char m_searchNameInputTxt[NAME_INPUT_MAX_LEN] = {};
	size_t m_searchNameIdx = -1;

	MainComponent* m_removeComp = nullptr;
	GameObject* m_deleteObject = nullptr;
	GameObject* m_deleteObjectAfterBreakDependencies = nullptr;

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

	bool m_isDrawingDebug = true;
	bool m_isDrawingInspectingObjectBasis = true;
	bool m_isDrawingInspectingObjectAABB = true;

	bool m_isDrawingInspectingObjectTransformEditingBasis = true;

	bool m_isHotDeserializingGameObjectFromFile = false;
	std::map<UUID, LoadedObjectFromFileData> m_loadFromFileObject;

	std::set<GameObject*> m_highlightingObjects;

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_inspectingObject);
		tracer->Trace(m_inspectingObjectData);
	}

	void OnObjectSelected(GameObject* obj);

	void RenderHierarchyPanelOf(GameObject* obj);
	void RenderHierarchyPanel();
	void RenderHierarchyPanelGameObjectsTree(GameObject* specified);
	void RenderInspectorPanel();

	void ShowCreateComponentPopup();
	void ShowCreateGameObjectPopup();

	Handle<GameObject> LoadGameObjectFromFile(const String& path, bool hotReload = true);

	Handle<GameObject> LoadStaticModelFromFile(const String& path);

	void ReindexObjects();
	void ReindexChildren(Array<Handle<GameObject>>& children);
	void IndexObject(GameObject* obj);

	void RenderObjectContextPopup(GameObject* obj);
	void BreakDependencies(GameObject* obj);

public:
	SceneEditorTab();

	void OnObjectsAdded(std::vector<GameObject*>& objects) override;
	void OnObjectsRemoved(std::vector<GameObject*>& objects) override;
	void OnRenderGUI() override;
	virtual void OnRenderMenuBar(const String& menuName) override;

	virtual void OnHotReloadGameObject(GameObject* startNewObj, GameObject* startOldObj, GameObject* currentNewObj, GameObject* currentOldObj);

	void OnRenderInGameDebugGraphics() override;
	
	void OnShow() override;
	void OnHide() override;
	void OnOpen() override;
	void OnClose() override;

	inline virtual String GetTabClassName() const override
	{
		return "SceneEditor";
	}

	void OnObjectDelete(GameObject* obj);
	void AddObjectToEditor(GameObject* obj);

	void ReloadCurrentInspectingObject();

public:
	inline auto& Lock()
	{
		return m_lock;
	}

public:
	void Inspect(ClassMetadata* metaData);
	virtual void WriteSaveDataToJson(Serializer* serializer, json& j);
	virtual void ReadSaveDataFromJson(Serializer* serializer, const json& j);
	virtual void OnRenderGameObjectContextMenu(GameObject* obj);

	virtual bool CheckCanBeDeleted(GameObject* obj);

public:
	void HighlightObject(GameObject* obj);
	void UnhighlightObject(GameObject* obj);

};

