#include "SimpleScene.h"

#include "Components/StaticObject.h"

#include "Engine/Engine.h"

#include "Engine/Controllers/CameraController.h"

#include "Components/AnimObject.h"

#include "Core/PostProcessor.h"
#include "Core/IRenderer.h"

#include "Engine/Scripting/Modules/JavaScript/v8/v8EngineWrapper.h"

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
	m_scriptEngine = new v8EngineWrapper();
	m_scriptEngine->SetupRuntime(engine, this);

	BeginUpdate(0);

	const auto deleter = [](SceneNode* node)
	{
		//std::cout << "\'" << SceneNode::NODE_TYPE_DESC[node->Type()] << "\' deleted.\n";
		delete node->RenderingObject().renderableObject;
	};

	const auto camDeleter = [](SceneNode* node)
	{
		//std::cout << "\'" << SceneNode::NODE_TYPE_DESC[node->Type()] << "\' deleted.\n";
		//delete node->RenderingObject().camera;
		delete (Controller*)node->Controller();
	};


	//============================================================================================
	auto sunLight = engine->Renderer()->LightSystem()->NewLight(LIGHT_TYPE::CSM_DIRECTIONAL_LIGHT, 0,
		0.50f, 0.3f, 0.02f, 0.8f, { 10,20,0 }, Vec3(0, -5, -3.4).Normalize(), Vec3(1, 1, 1));

	engine->Renderer()->LightSystem()->AddLight(sunLight);
	engine->Renderer()->LightSystem()->AddShadow(sunLight, SHADOW_MAP_QUALITY::HIGH);
	//follow cam
	engine->Renderer()->LightSystem()
		->GetLightExData<LightSystem::ExtraDataCSMDirLight>(sunLight)
		->Follow(
			&engine->Renderer()->GetTargetCamera()->ViewMatrix(), 
			&engine->Renderer()->GetTargetCamera()->ProjectionMatrix()
		);

	class LightController : public Controller
	{
	public:
		virtual bool Update(SceneQueryContext* context,
			SceneQueriedNode* node, const Mat4x4& globalTransform, Engine* engine) override
		{
			node->Blob().AsLight().dir = Vec3(0.0, sin(engine->FTime() * 0.2), cos(engine->FTime() * 0.2)).Normalize();
			context->FlushBackData(node);

			return true;
		};

		virtual void CallMethod(SceneQueryContext* context, SceneQueriedNode* node, const Mat4x4& globalTransform,
			int methodId, void* args, int argc, Engine* engine) {};
	};

	const auto lightDeleter = [](SceneNode* node)
	{
		delete (Controller*)node->Controller();
	};

	auto lightNode = NewLightNode();
	lightNode->Deleter() = lightDeleter;
	//lightNode->Controller() = new LightController();
	lightNode->RenderingObject().lightID = sunLight;
	lightNode->Blob().AsLight() = engine->Renderer()->LightSystem()->GetLight(sunLight);
	m_nodes.push_back(lightNode);


	auto cameraNode = NewCameraNode();
	cameraNode->Deleter() = camDeleter;
	//cameraNode->StateChange() = 0;
	cameraNode->Controller() = new CameraController(
		{ 0, 50, 0 }, { 0,50,50 }, ConvertToRadians(60), 0.5, 1000,
		engine->Renderer()->GetRenderWidth() / (float)engine->Renderer()->GetRenderHeight(),
		50.0f
	);
	cameraNode->RenderingObject().camera = engine->m_cam;
	m_nodes.push_back(cameraNode);


	IRenderableObject* obj = 0;
	SceneNode* node = 0;

	////============================================================================================
	//auto node = NewRenderableObjectNode();
	//node->Deleter() = deleter;
	//IRenderableObject* obj = new BasicObject(
	//	L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
	//	L"D:/KEngine/ResourceFile/temp_img/Copper.png",
	//	GetScaleMatrix(3, 3, 3)
	//);
	//obj->Transform() *= GetScaleMatrix(1, 5, 0.5f);
	//obj->Transform().SetPosition(0, 0, 10);
	//node->RenderingObject().renderableObject = obj;
	//node->Transform() = obj->Transform();
	//m_nodes.push_back(node);


	////============================================================================================
	//node = NewRenderableObjectNode();
	//node->Deleter() = deleter;
	//obj = new BasicObject(
	//	L"D:/KEngine/ResourceFile/model/simple_model/sphere.obj",
	//	L"D:/KEngine/ResourceFile/temp_img/Blue.png",
	//	GetScaleMatrix(3, 3, 3)
	//);
	//obj->SetPosition(15, 30, -20);
	//node->RenderingObject().renderableObject = obj;
	//node->Transform() = obj->Transform();
	//m_nodes.push_back(node);


	////============================================================================================
	//node = NewRenderableObjectNode();
	//node->Deleter() = deleter;
	//obj = new BasicObject(
	//	L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
	//	L"D:/KEngine/ResourceFile/temp_img/Copper.png",
	//	GetScaleMatrix(3, 3, 3)
	//);
	//obj->Transform().SetScale(1, 5, 0.5f);
	//obj->Transform().SetPosition(0, 0, 20);
	//node->RenderingObject().renderableObject = obj;
	//node->Transform() = obj->Transform();
	//m_nodes.push_back(node);


	////============================================================================================
	//node = NewRenderableObjectNode();
	//node->Deleter() = deleter;
	//obj = new BasicObject(
	//	L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
	//	L"D:/KEngine/ResourceFile/temp_img/Copper.png",
	//	GetScaleMatrix(3, 3, 3)
	//);
	//obj->Transform().SetScale(1, 5, 10.f);
	//obj->Transform().SetPosition(0, 0, -30);
	//node->RenderingObject().renderableObject = obj;
	//node->Transform() = obj->Transform();
	//m_nodes.push_back(node);


	////============================================================================================
	//node = NewRenderableObjectNode();
	//node->Deleter() = deleter;
	//obj = new BasicObject(
	//	L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
	//	L"D:/KEngine/ResourceFile/temp_img/Copper.png",
	//	GetScaleMatrix(3, 3, 3)
	//);
	//obj->Transform().SetScale(1, 5, 10.f);
	//obj->Transform().SetPosition(0, 0, 60);
	//node->RenderingObject().renderableObject = obj;
	//node->Transform() = obj->Transform();
	//m_nodes.push_back(node);


	////============================================================================================
	//node = NewRenderableObjectNode();
	//node->Deleter() = deleter;
	//obj = new BasicObject(
	//	L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
	//	L"D:/KEngine/ResourceFile/temp_img/Blue.png",
	//	GetScaleMatrix(3, 3, 3)
	//);
	//obj->Transform().SetScale(0.5f, 10, 32.0f);
	//obj->Transform().SetPosition(0, 40, 0);
	//node->RenderingObject().renderableObject = obj;
	//node->Transform() = obj->Transform();
	//m_nodes.push_back(node);


	////============================================================================================
	//node = NewRenderableObjectNode();
	//node->Deleter() = deleter;
	//obj = new BasicObject(
	//	L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
	//	L"D:/KEngine/ResourceFile/temp_img/Blue.png",
	//	GetScaleMatrix(3, 3, 3)
	//);
	//obj->Transform().SetScale(0.5f, 3, 10.0f);
	//obj->Transform().SetPosition(0, -10, 15);
	//node->RenderingObject().renderableObject = obj;
	//node->Transform() = obj->Transform();
	//m_nodes.push_back(node);


	////============================================================================================
	//node = NewRenderableObjectNode();
	//node->Deleter() = deleter;
	//obj = new BasicObject(
	//	L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
	//	L"D:/KEngine/ResourceFile/temp_img/Blue.png",
	//	GetScaleMatrix(3, 3, 3)
	//);
	//obj->Transform().SetScale(0.5f, 6, 20.0f);
	//obj->Transform() *= GetRotationAxisMatrix({ 0, 1, 0 }, PI / 2);
	//obj->Transform().SetPosition(-45, 0, 50);
	//node->RenderingObject().renderableObject = obj;
	//node->Transform() = obj->Transform();
	//m_nodes.push_back(node);


	//============================================================================================
	//plane
	node = NewRenderableObjectNode();
	node->Deleter() = deleter;
	obj = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/Plane.obj",
		L"D:/KEngine/ResourceFile/temp_img/white.png",
		GetScaleMatrix(1000, 1000, 1000)
	);
	obj->Transform().SetTranslation(0, -15, 0);
	node->RenderingObject().renderableObject = obj;
	node->Transform() = obj->Transform();
	m_nodes.push_back(node);


	////============================================================================================
	////sponza
	//node = NewRenderableObjectNode();
	//node->Deleter() = deleter;

	////std::vector<PBRMaterialPath> sponzaMaterials = {
	////		{ L"D:/KEngine/ResourceFile/temp_img/uvmap1.DDS", L"", L"", L"", L"" },
	////		//{ L"", L"", L"", L"", L"" }
	////};
	//obj = new PBRMultiMeshStaticObject(
	//	L"D:/KEngine/ResourceFile/model/McGuire/Sponza/sponza.obj",
	//	GetScaleMatrix(0.2, 0.2, 0.2)
	//);
	//node->RenderingObject().renderableObject = obj;
	//node->Transform() = obj->Transform();
	//m_nodes.push_back(node);


	////============================================================================================
	//node = NewRenderableObjectNode();
	//node->Deleter() = deleter;

	//std::vector<PBRMaterialPath> paths = {
	//		{ L"D:/KEngine/ResourceFile/model/character/Character Texture 256x256.png", L"", L"", L"", L"" }
	//};
	//auto animObject = new PBRMultiMeshAnimObject(
	//	L"D:/KEngine/ResourceFile/model/character/Character1.fbx",
	//	paths,
	//	GetScaleMatrix(0.02f, 0.02f, 0.02f)
	//);
	////animObject->Animator().SetAnimation(0);

	//obj = animObject;
	///*obj = new BasicObject(
	//	L"D:/KEngine/ResourceFile/model/character/Character Running.fbx",
	//	L"D:/KEngine/ResourceFile/model/character/Character Texture 256x256.png",
	//	GetScaleMatrix(0.02f, 0.02f, 0.02f)
	//);*/
	//obj->Transform() *= GetRotationYMatrix(PI / 2.0f);
	//obj->Transform().SetPosition(50, 0, 30);
	//node->RenderingObject().renderableObject = obj;
	//node->Transform() = obj->Transform();
	//m_nodes.push_back(node);


	for (auto& node : m_nodes)
	{
		if (node->Type() == SceneNode::RENDERABLE_NODE)
			node->m_aabb = node->RenderingObject().renderableObject->GetAABB();

		node->IncreRef();
	}

	EndUpdate(0);

	////=================================test post processing======================================
	//auto postproc = engine->Renderer()->PostProcessor();
	//
	//bloomSV = new ShaderVar(&bloomData, sizeof(bloomData));

	//constexpr static float DOWN_RES_FACTOR = 2.0f;

	//auto lastscene = postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::LIGHTED_SCENE);
	//auto screenSurface = postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::SCREEN_BUFFER);

	//auto w = engine->Renderer()->GetRenderWidth();
	//auto h = engine->Renderer()->GetRenderHeight();

	//auto output1 = postproc->GetTexture2D(Vec2(w / DOWN_RES_FACTOR, h / DOWN_RES_FACTOR));
	//auto layer2 = postproc->MakeLayer({ output1 });

	//auto gausianBlurPS = Resource::Get<PixelShader>(L"PostProcessing/Blur/GaussianBlur");
	//auto combinePS = Resource::Get<PixelShader>(L"PostProcessing/Blur/PSCombine");

	//struct Temp
	//{
	//	ShaderVar* buf;
	//	float w;
	//	float h;
	//};

	//static Temp _opaque = {};
	//_opaque.buf = bloomSV;
	//_opaque.w = w;
	//_opaque.h = h;

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
}

SimpleScene::~SimpleScene()
{
	delete m_scriptEngine;

	BeginUpdate(0);

	for (auto& node : m_nodes)
	{
		RefCounted::Release(&node);
	}

	EndUpdate(0);
}

void SimpleScene::Query2D(SceneQueryContext* context, Rect2D* area, std::vector<NodeId>& output)
{
}

void SimpleScene::Query2D(SceneQueryContext* context, AARect2D* area, std::vector<NodeId>& output)
{
}

void SimpleScene::Query2D(SceneQueryContext* context, Trapezoid* area, std::vector<NodeId>& output)
{
}

void SimpleScene::Query2D(SceneQueryContext* context, Circle* area, std::vector<NodeId>& output)
{
}

void SimpleScene::Query3D(SceneQueryContext* context, Box* bounding, std::vector<NodeId>& output)
{
}

void SimpleScene::Query3D(SceneQueryContext* context, AABB* bounding, std::vector<NodeId>& output)
{
}

void SimpleScene::Query3D(SceneQueryContext* context, Frustum* bounding, std::vector<NodeId>& output)
{
	BeginQuery(context);

	for (auto& node : m_nodes)
	{
		output.push_back(context->NewFromSceneNode(*node));
	}

	EndQuery(context);
}

void SimpleScene::Query3D(SceneQueryContext* context, Sphere* bounding, std::vector<NodeId>& output)
{
}

void SimpleScene::LoadFromFile(const std::string& path)
{
}

void SimpleScene::Query3DImmutableNodes(SceneQueryContext* context, Math::Box* bounding, std::vector<size_t, std::allocator<size_t>>& output)
{
}

void SimpleScene::Query3DImmutableNodes(SceneQueryContext* context, Math::AABB* bounding, std::vector<size_t, std::allocator<size_t>>& output)
{
}

void SimpleScene::Query3DImmutableNodes(SceneQueryContext* context, Math::Frustum* bounding, std::vector<size_t, std::allocator<size_t>>& output)
{
}

void SimpleScene::Query3DImmutableNodes(SceneQueryContext* context, Math::Sphere* bounding, std::vector<size_t, std::allocator<size_t>>& output)
{
}

void SimpleScene::Query3DMutableNodes(SceneQueryContext* context, Math::Box* bounding, std::vector<size_t, std::allocator<size_t>>& output)
{
}

void SimpleScene::Query3DMutableNodes(SceneQueryContext* context, Math::AABB* bounding, std::vector<size_t, std::allocator<size_t>>& output)
{
}

void SimpleScene::Query3DMutableNodes(SceneQueryContext* context, Math::Frustum* bounding, std::vector<size_t, std::allocator<size_t>>& output)
{
}

void SimpleScene::Query3DMutableNodes(SceneQueryContext* context, Math::Sphere* bounding, std::vector<size_t, std::allocator<size_t>>& output)
{
}

void SimpleScene::AddNodes(SceneNode** nodes, size_t count)
{
	m_nodes.insert(m_nodes.end(), &nodes[0], &nodes[count]);
	for (size_t i = 0; i < count; i++)
	{
		nodes[i]->IncreRef();
	}
}

void SimpleScene::RemoveNodes(SceneNode** nodes, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		auto it = std::find(m_nodes.begin(), m_nodes.end(), nodes[i]);
		auto* node = *it;
		RefCounted::Release(&node);
		m_nodes.erase(it);
	}
}
