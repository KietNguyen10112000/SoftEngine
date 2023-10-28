#include "Plugins/Bridge/PluginImpl.h"

#include "Runtime/Runtime.h"
#include "Scene/Scene.h"
#include "MainSystem/Rendering/RenderingSystem.h"

class Editor
{

};

class EditorContext
{
public:
	void OnGUI()
	{
		bool show = true;
		ImGui::ShowDemoWindow(&show);
	}

};

void RegisterSerializables()
{
	
}

void Initialize(Runtime* runtime)
{
	auto editor		= mheap::New<Editor>();
	auto editorId	= runtime->GenericStorage()->Store(editor);

	runtime->EventDispatcher()->AddListener(Runtime::EVENT_SCENE_CREATED,
		[](Runtime* runtime, int argc, void** argv, ID editorId)
		{
			auto scene				= (Scene*)argv[0];
			auto editorContext		= mheap::New<EditorContext>();
			auto editorContextId	= scene->GenericStorage()->Store(editorContext);

			scene->GetRenderingSystem()->EventDispatcher()->AddListener(RenderingSystem::EVENT_RENDER_GUI,
				[](RenderingSystem* renderingSystem, int argc, void** argv, ID editorContextId)
				{
					auto editorContext = renderingSystem->GetScene()->GenericStorage()->Access<EditorContext>(editorContextId);
					editorContext->OnGUI();
				},
				editorContextId
			);
		},
		editorId
	);
}

void Finalize(Runtime* runtime)
{

}