#include "SimpleScene.h"

#include "Components/StaticObject.h"

#include "Engine/Engine.h"

#include "Engine/Controllers/CameraController.h"

#include "Components/AnimObject.h"

#include "Core/PostProcessor.h"
#include "Core/IRenderer.h"

#include "Engine/Scripting/Modules/JavaScript/v8/v8EngineWrapper.h"
#include "Engine/Physics/Modules/Bullet/BulletEngine.h"
#include "Engine/Physics/PhysicsObject.h"

#include "Engine/Random.h"

#if defined(_DEBUG) || defined(LOCAL_RELEASE)
#define RESOURCES_WDIR L"../../../../Resources/"
#else
#define RESOURCES_WDIR L"./Resources/"
#endif // _DEBUG



struct GaussianBlurData
{
	uint32_t type;
	float width;
	float height;
	float downscale;
};

struct LumaData
{
	float thres;
	Vec3 factor;
};

struct BloomData
{
	union
	{
		GaussianBlurData gaussian = {};
		LumaData luma;
	};
};

ShaderVar* bloomSV = 0;
BloomData bloomData;


SimpleScene::SimpleScene(Engine* engine) : Scene(engine)
{
	//m_scriptEngine = new v8EngineWrapper();
	//m_scriptEngine->SetupRuntime(engine, this);

	m_physicsEngine = new BulletEngine();

	InterlockedAcquire(0);


	//============================================================================================
	//Vec3(0, -5, -3.4)
	auto sunLight = engine->Renderer()->LightSystem()->NewLight(LIGHT_TYPE::CSM_DIRECTIONAL_LIGHT, 0,
		0.01f, 0.3f, 0.02f, 0.8f, { 10,20,0 }, Vec3(-6, -5, 0).Normalize(), Vec3(1, 1, 1));

	engine->Renderer()->LightSystem()->AddLight(sunLight);
	engine->Renderer()->LightSystem()->AddShadow(sunLight, SHADOW_MAP_QUALITY::_2K);

	float csmSeparateThres[] = { 0.25f, 0.25f, 0.25f, 0.25f };
	//follow cam
	engine->Renderer()->LightSystem()
		->GetLightExData<LightSystem::ExtraDataCSMDirLight>(sunLight)
		->SetMaxLength(300)
		//->SetSeparateThread(csmSeparateThres)
		->Follow(
			&engine->Renderer()->GetTargetCamera()->ViewMatrix(), 
			&engine->Renderer()->GetTargetCamera()->ProjectionMatrix()
		);

	engine->Renderer()->LightSystem()->m_info.offsetPixelLightFactor.x = 0.5f;


	//Vec3(cos(engine->FTime() * 0.2), sin(engine->FTime() * 0.2), 0.0f).Normalize();
	class LightController : public Controller
	{
	public:
		Vec3 m_currentDir = {};

	public:
		virtual void Update(Engine* engine)
		{
			m_currentDir = Vec3(cos(engine->FTime() * 0.2), sin(engine->FTime() * 0.2), 0.0f).Normalize();
		};

		virtual void WriteTo(SceneObject* object) 
		{
			object->ExternalData().As<Light>().dir = m_currentDir;
		};

		virtual void WriteToShared(SceneSharedObject* object)
		{
			object->ExternalData().As<Light>().dir = m_currentDir;
		};
	};


	auto light = NewLight();
	//lightNode->Controller() = new LightController();
	light->RenderingObject().lightID = sunLight;
	light->ExternalData().As<Light>() = engine->Renderer()->LightSystem()->GetLight(sunLight);
	m_objects.push_back(light);


	auto camera = NewCamera();
	auto camCtrl = new CameraController(
		{ 10, 10, 10 }, { 0,0,0 }, ConvertToRadians(60), 0.1f, 10000.0f,
		engine->Renderer()->GetRenderWidth() / (float)engine->Renderer()->GetRenderHeight(),
		15.0f, 0.12f
	);
	camera->Controller() = camCtrl;
	camera->RenderingObject().camera = engine->m_cam;
	m_objects.push_back(camera);

	SetDebugCamera(camera);


	IRenderableObject* obj = 0;
	SceneObject* object;


	Transform transform;
	Shape shape = Shape::Get<SphereShape>(15);
	PhysicsMaterial physicsMaterial;
	std::vector<PhysicsObject*> physicsObjs;

	//============================================================================================

	for (int64_t i = 0; i < 5; i++)
	{
		for (int64_t j = 0; j < 5; j++)
		{
			for (int64_t k = 0; k < 120; k++)
			{
				object = NewObject();


				// rendering object
				obj = new BasicObject(
					RESOURCES_WDIR L"Models/sphere.obj",
					RESOURCES_WDIR L"Images/uvmap1.DDS",
					GetScaleMatrix(1, 1, 1)
				);
				obj->Transform().SetPosition((i - 2.5f) * 2.0f, 30 + k * 2.0f, (j - 2.5f) * 2.0f);
				object->RenderingObject().renderableObject = obj;
				object->Transform() = obj->Transform();


				// physics object
				physicsMaterial.mass = Random::Float(0.1f, 50.0f);
				physicsMaterial.friction = Random::Float(0.5f, 1.0f);
				physicsMaterial.restitution = Random::Float(0.5f, 1.0f);
				shape = Shape::Get<SphereShape>(1);
				transform.FromMatrix(obj->Transform());
				object->PhysicsObject() = GetPhysicsEngine()->MakeObject(transform, shape, physicsMaterial);


				physicsObjs.push_back(object->PhysicsObject());
				m_objects.push_back(object);
			}
		}
	}

	////============================================================================================

	Mat4x4 temp = GetTranslationMatrix(15, 0, 0);
	float deltaAngle = 2 * PI / 30.0f;

	for (size_t i = 0; i < 30; i++)
	{
		object = NewObject();


		// rendering object
		obj = new BasicObject(
			RESOURCES_WDIR L"Models/cube1.obj",
			RESOURCES_WDIR L"Images/blue.png",
			GetScaleMatrix(1, 10, 1)
		);
		obj->Transform().SetPosition(0, 5, 0) *= temp;
		object->RenderingObject().renderableObject = obj;
		object->Transform() = obj->Transform();


		// physics object
		physicsMaterial.mass = 0.0f;
		physicsMaterial.restitution = Random::Float(0.5f, 1.0f);
		shape = Shape::Get<BoxShape>(2, 20, 2);
		transform.FromMatrix(obj->Transform());
		object->PhysicsObject() = GetPhysicsEngine()->MakeObject(transform, shape, physicsMaterial);


		physicsObjs.push_back(object->PhysicsObject());
		m_objects.push_back(object);

		temp *= GetRotationYMatrix(deltaAngle);
	}
	


	//============================================================================================
	//plane
	object = NewObject();

	obj = new BasicObject(
		RESOURCES_WDIR L"Models/Plane.obj",
		RESOURCES_WDIR L"Images/white.png",
		GetScaleMatrix(1000, 1, 1000)
	);
	obj->Transform().SetTranslation(0, -2, 0);
	object->RenderingObject().renderableObject = obj;
	object->Transform() = obj->Transform();

	physicsMaterial.mass = 0.0f;
	physicsMaterial.restitution = 0.9f;
	shape = Shape::Get<BoxShape>(1000, 0.25f, 1000);
	transform.FromMatrix(obj->Transform());
	object->PhysicsObject() = GetPhysicsEngine()->MakeObject(transform, shape, physicsMaterial);
	physicsObjs.push_back(object->PhysicsObject());

	m_objects.push_back(object);


	//GetPhysicsEngine()->SetGravity({0, -9.8 * 15.0f, 0});
	GetPhysicsEngine()->AddObjects(physicsObjs);


	object = NewObject();
	obj = new BasicObject(
		RESOURCES_WDIR L"Models/cube1.obj",
		RESOURCES_WDIR L"Images/blue.png",
		GetScaleMatrix(1, 10, 1)
	);
	obj->Transform().SetPosition(-300, 5, 0);
	object->RenderingObject().renderableObject = obj;
	object->Transform() = obj->Transform();
	m_objects.push_back(object);


	InterlockedRelease(0);

	////=================================test post processing======================================
	//auto postproc = engine->Renderer()->PostProcessor();
	//
	//bloomSV = new ShaderVar(&bloomData, sizeof(bloomData));

	constexpr static float DOWN_RES_FACTOR = 2.0f;

	//auto lastscene = postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::LIGHTED_SCENE);
	//auto screenSurface = postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::SCREEN_BUFFER);

	auto w = engine->Renderer()->GetRenderWidth();
	auto h = engine->Renderer()->GetRenderHeight();

	//auto output1 = postproc->GetTexture2D(Vec2(w / DOWN_RES_FACTOR, h / DOWN_RES_FACTOR));
	//auto layer2 = postproc->MakeLayer({ output1 });

	//auto gausianBlurPS = Resource::Get<PixelShader>(L"PostProcessing/Blur/GaussianBlur");
	//auto combinePS = Resource::Get<PixelShader>(L"PostProcessing/Blur/PSCombine");

	struct Temp
	{
		ShaderVar* buf;
		float w;
		float h;
	};

	static Temp _opaque = {};
	_opaque.buf = bloomSV;
	_opaque.w = w;
	_opaque.h = h;

	////god rays post program
	//auto godRays = postproc->MakeProgram("God Rays");
	//auto godRayOutput0 = postproc->GetTexture2D(Vec2(w / DOWN_RES_FACTOR, h / DOWN_RES_FACTOR));
	//auto godRayUpScale = postproc->GetTexture2D(Vec2(w, h));

	//auto scenePosition = postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::POSITION_AND_SPECULAR);
	//auto sceneNormal = postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::NORMAL_AND_SHININESS);

	//auto godRaysInputLayer = postproc->MakeLayer({ scenePosition });

	//auto godRaysLayer1 = postproc->MakeLayer({ godRayOutput0, lastscene });

	//auto godRaysUpscaleLayer = postproc->MakeLayer({ godRayUpScale, lastscene });

	//auto screenLayer = postproc->MakeLayer({ screenSurface });

	//auto godRaysPS = Resource::Get<PixelShader>(L"PostProcessing/LightEffect/GodRays/GodRays_PS");

	//auto godRaysCombinePS = Resource::Get<PixelShader>(L"PostProcessing/LightEffect/GodRays/Combine");

	//if (!godRays->IsCrafted())
	//{
	//	godRays->Append(godRaysInputLayer);

	//	godRays->Append(godRaysPS, &_opaque,
	//		[](IRenderer*, void* opaque)
	//		{
	//			ShaderVar* buf = _opaque.buf;
	//			bloomData.gaussian.type = 0;
	//			bloomData.gaussian.width = _opaque.w / DOWN_RES_FACTOR;
	//			bloomData.gaussian.height = _opaque.h / DOWN_RES_FACTOR;
	//			bloomData.gaussian.downscale = DOWN_RES_FACTOR;
	//			buf->Update(&bloomData, sizeof(bloomData));
	//			RenderPipeline::PSSetVar(buf, 2);
	//		}
	//	);

	//	//for (size_t i = 0; i < 5; i++)
	//	//{
	//	godRays->Append(godRaysLayer1);
	//	//godRays->Append(copyPS);
	//	//godRays->Append(godRaysUpscaleLayer);
	//	//=================2 pass Gaussian Blur=============================
	//	godRays->Append(gausianBlurPS, &_opaque,
	//		[](IRenderer*, void* opaque)
	//		{
	//			ShaderVar* buf = _opaque.buf;
	//			bloomData.gaussian.type = 0;
	//			bloomData.gaussian.width = _opaque.w / DOWN_RES_FACTOR;
	//			bloomData.gaussian.height = _opaque.h / DOWN_RES_FACTOR;
	//			buf->Update(&bloomData, sizeof(bloomData));
	//		}
	//	);

	//	//godRays->Append(godRaysUpscaleLayer);
	//	godRays->Append(layer2);

	//	godRays->Append(gausianBlurPS, &_opaque,
	//		[](IRenderer* renderer, void* opaque)
	//		{
	//			ShaderVar* buf = _opaque.buf;
	//			bloomData.gaussian.type = 1;
	//			buf->Update(&bloomData, sizeof(bloomData));
	//		}
	//	);
	//	//=================End 2 pass Gaussian Blur==========================
	//	//}

	//	//godRays->Append(godRaysUpscaleLayer);
	//	godRays->Append(godRaysLayer1);

	//	godRays->Append(godRaysCombinePS);
	//	godRays->Append(screenLayer);

	//	auto ret = godRays->Craft();
	//	if (ret != "OK")
	//	{
	//		exit(2);
	//	}
	//}

	//auto postprocChain = postproc->MakeProcessChain("Bloom Effect");

	//if (!postprocChain->IsCrafted())
	//{	
	//	postprocChain->Append(
	//		{
	//			// programs
	//			{
	//				//re-run
	//				{ godRays, true }
	//			},

	//			{
	//				{
	//					//program, index
	//					{ {0, 0}, {0, 0} }
	//				}
	//			}
	//		}
	//	);
	//	
	//	auto ret = postprocChain->Craft();
	//	/*if (ret != "OK")
	//	{
	//		exit(2);
	//	}*/
	//}
	//

	//postproc->SetProcessChain(postprocChain);

	//Resource::Release(&gausianBlurPS);
	//Resource::Release(&combinePS);

	//Resource::Release(&godRaysPS);
	//Resource::Release(&godRaysCombinePS);


	////==================================Test SSR==========================================================
	////auto w					= engine->Renderer()->GetRenderWidth();
	////auto h					= engine->Renderer()->GetRenderHeight();
	//auto postproc			= engine->Renderer()->PostProcessor();

	//auto positionMap		= postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::POSITION_AND_SPECULAR);
	//auto normalMap			= postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::NORMAL_AND_SHININESS);
	//auto lastscene			= postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::LIGHTED_SCENE);
	//auto screenSurface		= postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::SCREEN_BUFFER);

	//auto ssrShader			= Resource::Get<PixelShader>(L"PostProcessing/SSR/SSR");
	//auto ssrCombineShader	= Resource::Get<PixelShader>(L"PostProcessing/SSR/Combine");

	//auto gausianBlurShader	= Resource::Get<PixelShader>(L"PostProcessing/Blur/BoxBlur");
	////auto combineShader		= Resource::Get<PixelShader>(L"PostProcessing/Blur/PSCombine");


	//
	//auto fullres			= postproc->GetTexture2D(Vec2(w / DOWN_RES_FACTOR, h / DOWN_RES_FACTOR));

	//auto inputLayer			= postproc->MakeLayer({ positionMap, normalMap, lastscene });
	//auto midLayer			= postproc->MakeLayer({ fullres, lastscene });
	//auto outputLayer		= postproc->MakeLayer({ screenSurface });

	//// half resolution
	//auto blurOutput1		= postproc->GetTexture2D(Vec2(w / DOWN_RES_FACTOR, h / DOWN_RES_FACTOR));
	//auto blurOutput2		= postproc->GetTexture2D(Vec2(w / DOWN_RES_FACTOR, h / DOWN_RES_FACTOR));
	//auto blurLayer1			= postproc->MakeLayer({ blurOutput1 });
	//auto blurLayer2			= postproc->MakeLayer({ blurOutput2 });

	//auto ssrProgram			= postproc->MakeProgram("SSR");

	//if (!ssrProgram->IsCrafted())
	//{
	//	ssrProgram->Append(inputLayer);
	//	ssrProgram->Append(ssrShader);

	//	ssrProgram->Append(blurLayer1);

	//	//=================2 pass Gaussian Blur=============================
	//	ssrProgram->Append(gausianBlurShader, &_opaque,
	//		[](IRenderer*, void* opaque)
	//		{
	//			ShaderVar* buf = _opaque.buf;
	//			bloomData.gaussian.type = 0;
	//			bloomData.gaussian.width = _opaque.w / DOWN_RES_FACTOR;
	//			bloomData.gaussian.height = _opaque.h / DOWN_RES_FACTOR;
	//			buf->Update(&bloomData, sizeof(bloomData));
	//			RenderPipeline::PSSetVar(buf, 2);
	//		}
	//	);

	//	ssrProgram->Append(blurLayer2);

	//	ssrProgram->Append(gausianBlurShader, &_opaque,
	//		[](IRenderer* renderer, void* opaque)
	//		{
	//			ShaderVar* buf = _opaque.buf;
	//			bloomData.gaussian.type = 1;
	//			buf->Update(&bloomData, sizeof(bloomData));
	//		}
	//	);
	//	//=================2 pass Gaussian Blur=============================


	//	ssrProgram->Append(midLayer);
	//	ssrProgram->Append(ssrCombineShader);

	//	ssrProgram->Append(outputLayer);

	//	ssrProgram->Craft();
	//}


	//auto postprocChain		= postproc->MakeProcessChain("SSR");

	//if (!postprocChain->IsCrafted())
	//{
	//	postprocChain->Append(
	//		{
	//			// programs
	//			{
	//				//re-run
	//				{ ssrProgram, true }
	//			},

	//			{
	//				{
	//					//program, index
	//					{ {0, 0}, {0, 0} }
	//				}
	//			}
	//		}
	//	);

	//	auto ret = postprocChain->Craft();
	//}
	//
	//postproc->SetProcessChain(postprocChain);

	//Resource::Release(&ssrShader);
	//Resource::Release(&ssrCombineShader);

	//Resource::Release(&gausianBlurShader);
}

SimpleScene::~SimpleScene()
{
	delete m_scriptEngine;
	delete m_physicsEngine;

	InterlockedAcquire(0);

	for (auto& obj : m_objects)
	{
		delete obj;
	}

	m_sharedObjects.clear();

	InterlockedRelease(0);

	delete bloomSV;
}

void SimpleScene::Query3DObjects(SceneQueryContext* context, Box* bounding, SceneObjectList& output)
{
}

void SimpleScene::Query3DObjects(SceneQueryContext* context, AABB* bounding, SceneObjectList& output)
{
}

void SimpleScene::Query3DObjects(SceneQueryContext* context, Frustum* bounding, SceneObjectList& output)
{
	for (auto& obj : m_objects)
	{
		output.push_back(obj);
	}
	QueryContextAcquireReadingObjects(context, output);
}

void SimpleScene::Query3DObjects(SceneQueryContext* context, Sphere* bounding, SceneObjectList& output)
{
}

void SimpleScene::Query3DSharedObjects(SceneQueryContext* context, Box* bounding, SceneSharedObjectList& output)
{
}

void SimpleScene::Query3DSharedObjects(SceneQueryContext* context, AABB* bounding, SceneSharedObjectList& output)
{
}

void SimpleScene::Query3DSharedObjects(SceneQueryContext* context, Frustum* bounding, SceneSharedObjectList& output)
{
	for (auto& obj : m_sharedObjects)
	{
		output.push_back(obj);
	}
	QueryContextAcquireReadingObjects(context, output);
}

void SimpleScene::Query3DSharedObjects(SceneQueryContext* context, Sphere* bounding, SceneSharedObjectList& output)
{
}

void SimpleScene::AddSharedObjects(SharedObject* objects, size_t count)
{
}

void SimpleScene::RemoveSharedObjects(SharedObject* objects, size_t count)
{
}

void SimpleScene::AddObjects(SceneObject* nodes, size_t count)
{
}

void SimpleScene::RemoveObjects(SceneObject* nodes, size_t count)
{
}

void SimpleScene::FilterObjects(SceneObjectList& output, FilterFunction func)
{
}

void SimpleScene::FilterSharedObjects(SceneSharedObjectList& output, FilterFunction func)
{
}



