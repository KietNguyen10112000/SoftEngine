#include "Plugins/Bridge/PluginImpl.h"

#include "Runtime/Runtime.h"
#include "Scene/Scene.h"
#include "MainSystem/Rendering/RenderingSystem.h"

#include "EditorContext.h"

void RegisterSerializables()
{
	
}

void Initialize(Runtime* runtime)
{
	runtime->EventDispatcher()->AddListener(Runtime::EVENT_SCENE_CREATED,
		[](Runtime* runtime, int argc, void** argv, ID editorId)
		{
			auto scene				= (Scene*)argv[0];
			auto editorContext		= mheap::New<EditorContext>();
			auto editorContextId	= scene->GenericStorage()->Store(editorContext);

			editorContext->m_scene = scene;

			scene->GetRenderingSystem()->EventDispatcher()->AddListener(RenderingSystem::EVENT_RENDER_GUI,
				[](RenderingSystem* renderingSystem, int argc, void** argv, ID editorContextId)
				{
					auto editorContext = renderingSystem->GetScene()->GenericStorage()->Access<EditorContext>(editorContextId);
					editorContext->Lock().lock();
					editorContext->OnRenderGUI();
					editorContext->Lock().unlock();
				},
				editorContextId
			);

			scene->EventDispatcher()->AddListener(Scene::EVENT_OBJECTS_ADDED,
				[](Scene* scene, int argc, void** argv, ID editorContextId)
				{
					auto objs = (std::vector<GameObject*>*)argv[0];
					auto editorContext = scene->GenericStorage()->Access<EditorContext>(editorContextId);
					editorContext->Lock().lock();
					editorContext->OnObjectsAdded(*objs);
					editorContext->Lock().unlock();
				},
				editorContextId
			);

			scene->EventDispatcher()->AddListener(Scene::EVENT_OBJECTS_REMOVED,
				[](Scene* scene, int argc, void** argv, ID editorContextId)
				{
					auto objs = (std::vector<GameObject*>*)argv[0];
					auto editorContext = scene->GenericStorage()->Access<EditorContext>(editorContextId);
					editorContext->Lock().lock();
					editorContext->OnObjectsRemoved(*objs);
					editorContext->Lock().unlock();
				},
				editorContextId
			);
		},
		0
	);
}

void Finalize(Runtime* runtime)
{

}