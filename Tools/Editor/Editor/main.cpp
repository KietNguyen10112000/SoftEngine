#include "Plugins/Bridge/PluginImpl.h"

#ifdef GetClassName
#undef GetClassName
#endif // GetClassName

#include "Runtime/Runtime.h"
#include "Scene/Scene.h"
#include "MainSystem/Rendering/RenderingSystem.h"

#include "EditorContext.h"
#include "DataInspector.h"
#include "ComponentInspector.h"
#include "EditorTabFactory.h"
#include "AnimatorEditorTabFactory.h"
#include "SceneEditorTabFactory.h"
#include "GameObjectEditorTabFactory.h"

#include "ScriptList.h"

#include "EditorFont.h"
#include "EditorSettings.h"


void RegisterSerializables()
{
	InitializeScriptList();
}

void Initialize(Runtime* runtime)
{
	/*{
		Transform parent0;
		parent0.Position() = { 10,10,2 };
		parent0.Rotation() = Mat4::Rotation(Vec3::Y_AXIS, PI / 3.0f);

		Transform parent1;
		parent1.Position() = { 5,4,8 };
		parent1.Rotation() = Mat4::Rotation(Vec3::X_AXIS, PI / 3.0f);

		Transform child0;
		child0.Position() = { 5,5,5 };
		child0.Rotation() = Mat4::Rotation(Vec3::Y_AXIS, PI / 2.0f);

		Transform child1;
		child1.Position() = { 3,3,2 };
		child1.Rotation() = Mat4::Rotation(Vec3::Z_AXIS, -PI / 2.0f);

		float blendFactor = 0.2345f;

		Mat4 ret0;
		{
			Transform p;
			p.Position() = Lerp(parent0.Position(), parent1.Position(), blendFactor);
			p.Rotation() = SLerp(parent0.Rotation(), parent1.Rotation(), blendFactor);

			Transform c;
			c.Position() = Lerp(child0.Position(), child1.Position(), blendFactor);
			c.Rotation() = SLerp(child0.Rotation(), child1.Rotation(), blendFactor);

			ret0 = c.ToTransformMatrix() * p.ToTransformMatrix();
		}

		Mat4 ret1;
		{
			auto m0 = child0.ToTransformMatrix() * parent0.ToTransformMatrix();
			auto m1 = child1.ToTransformMatrix() * parent1.ToTransformMatrix();
			ret1 = Lerp(m0, m1, blendFactor);
		}

		int x = 3;
	}*/

	EditorSettings::SingletonInitialize();
	EditorFont::SingletonInitialize();

	DataInspector::Initialize();
	ComponentInspector::SingletonInitialize();
	EditorTabFactoryManager::SingletonInitialize();

	runtime->EventDispatcher()->AddListener(Runtime::EVENT_SCENE_CREATED,
		[](Runtime* runtime, int argc, void** argv, ID editorId)
		{
			auto scene = (Scene*)argv[0];

			if (EditorContext::s_instance == nullptr)
			{
				auto editorContext = mheap::New<EditorContext>(scene);
				auto editorContextId = Runtime::Get()->GenericStorage()->Store(editorContext);
				EditorContext::s_instance = editorContext;
				EditorContext::s_instance->m_runTimeId = editorContextId;

				{
					auto factory = EditorTabFactoryManager::Get()->GetFactory<AnimatorEditorTabFactory>();

					factory->m_modelPath = "Editor/AnimatorEditor/Character.AnimatorEditor";
					auto tab = factory->CreateInstance();
					EditorContext::s_instance->RunTab(tab);

					EditorContext::s_instance->CloseTab(EditorContext::s_instance->GetCurrentTab());
				}

				/*{
					auto factory = EditorTabFactoryManager::Get()->GetFactory<SceneEditorTabFactory>();

					factory->m_filePath = "Editor/SceneEditor/Test2.json";
					auto tab = factory->CreateInstance();
					EditorContext::s_instance->RunTab(tab);

					EditorContext::s_instance->CloseTab(EditorContext::s_instance->GetCurrentTab());
				}*/

				/*{
					auto factory = EditorTabFactoryManager::Get()->GetFactory<GameObjectEditorTabFactory>();

					factory->m_filePath = "Editor/GameObjectEditor/Chair.GameObjectEditor";
					auto tab = factory->CreateInstance();
					EditorContext::s_instance->RunTab(tab);

					EditorContext::s_instance->CloseTab(EditorContext::s_instance->GetCurrentTab());
				}*/

				/*{
					auto factory = EditorTabFactoryManager::Get()->GetFactory<GameObjectEditorTabFactory>();

					factory->m_filePath = "Editor/GameObjectEditor/Table_0.GameObjectEditor";
					auto tab = factory->CreateInstance();
					EditorContext::s_instance->RunTab(tab);

					EditorContext::s_instance->CloseTab(EditorContext::s_instance->GetCurrentTab());
				}*/

				/*{
					auto factory = EditorTabFactoryManager::Get()->GetFactory<GameObjectEditorTabFactory>();

					factory->m_filePath = "Editor/GameObjectEditor/Character2.GameObjectEditor";
					auto tab = factory->CreateInstance();
					EditorContext::s_instance->RunTab(tab);

					EditorContext::s_instance->CloseTab(EditorContext::s_instance->GetCurrentTab());
				}*/

				/*{
					auto factory = EditorTabFactoryManager::Get()->GetFactory<GameObjectEditorTabFactory>();

					factory->m_filePath = "Editor/GameObjectEditor/Test.GameObjectEditor";
					auto tab = factory->CreateInstance();
					EditorContext::s_instance->RunTab(tab);

					EditorContext::s_instance->CloseTab(EditorContext::s_instance->GetCurrentTab());
				}*/
			}

			ID editorContextId = EditorContext::s_instance->m_runTimeId;

			/*scene->EventDispatcher()->AddListener(Scene::EVENT_BEGIN_RUNNING,
				[](Scene* scene, int argc, void** argv, ID editorContextId)
				{
					auto obj = mheap::New<GameObject>();
					obj->Name() = "#editor_TestScript";
					obj->NewComponent<TestScript>();
					scene->AddObject(obj);
				}
			);*/

			if (EditorContext::s_instance->m_tabHolder)
			{
				//assert(EditorContext::s_instance->m_tabHolder->m_scene == nullptr);
				if (EditorContext::s_instance->m_tabHolder->m_scene != nullptr)
				{
					assert(EditorContext::s_instance->m_tabHolder->m_scene == scene);
				}
				EditorContext::s_instance->m_tabHolder->m_scene = scene;
			}

			scene->GetRenderingSystem()->EventDispatcher()->AddListener(RenderingSystem::EVENT_RENDER_GUI,
				[](RenderingSystem* renderingSystem, int argc, void** argv, ID editorContextId)
				{
					auto editorContext = Runtime::Get()->GenericStorage()->Access<EditorContext>(editorContextId);
					editorContext->Lock().lock();
					editorContext->OnRenderGUI();
					editorContext->Lock().unlock();
				},
				editorContextId
			);

			scene->GetRenderingSystem()->EventDispatcher()->AddListener(RenderingSystem::EVENT_END_RENDER_CAMERA,
				[](RenderingSystem* renderingSystem, int argc, void** argv, ID editorContextId)
				{
					auto editorContext = Runtime::Get()->GenericStorage()->Access<EditorContext>(editorContextId);
					editorContext->Lock().lock();
					editorContext->OnRenderInGameDebugGraphics();
					editorContext->Lock().unlock();
				},
				editorContextId
			);

			scene->EventDispatcher()->AddListener(Scene::EVENT_OBJECTS_ADDED,
				[](Scene* scene, int argc, void** argv, ID editorContextId)
				{
					auto objs = (std::vector<GameObject*>*)argv[0];
					auto editorContext = Runtime::Get()->GenericStorage()->Access<EditorContext>(editorContextId);

					editorContext->Lock().lock();
					editorContext->OnObjectsAdded(*objs, scene);
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
					editorContext->OnObjectsRemoved(*objs, scene);
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
	//Runtime::Get()->GenericStorage()->Remove(EditorContext::GetInstance()->m_runTimeId);
	EditorContext::GetInstance()->OnFinalize();

	ComponentInspector::SingletonFinalize();
	EditorTabFactoryManager::SingletonFinalize();

	EditorFont::SingletonFinalize();
	EditorSettings::SingletonFinalize();
}