#pragma once
#include "EditorTab.h"

namespace soft
{
	class GameObject;
	class AnimatorSkeletalArray;
	class Scene;
}

namespace ax
{
	namespace NodeEditor
	{
		struct EditorContext;
	}
}

class AnimatorEditorTab : public EditorTab
{
public:
	String m_modelPath;
	Handle<GameObject> m_object;
	Handle<GameObject> m_cam;
	Handle<ClassMetadata> m_objMetadata;
	Handle<AnimatorSkeletalArray> m_animator;

	ax::NodeEditor::EditorContext* m_nodeEditorCtx = nullptr;

	ID m_onSaveListenerId = INVALID_ID;

	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_object);
		tracer->Trace(m_cam);
		tracer->Trace(m_objMetadata);
		tracer->Trace(m_animator);
	}

	AnimatorEditorTab(const String& modelPath, Scene* scene);

	// Inherited via EditorTab
	void OnObjectsAdded(std::vector<GameObject*>& objects) override;
	void OnObjectsRemoved(std::vector<GameObject*>& objects) override;
	void OnRenderGUI() override;
	void OnRenderInGameDebugGraphics() override;
	void OnShow() override;
	void OnHide() override;
	void OnOpen() override;
	void OnClose() override;

private:
	void RenderBluePrintPanel();

};

