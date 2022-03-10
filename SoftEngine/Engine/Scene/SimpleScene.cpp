#include "SimpleScene.h"

#include "Components/StaticObject.h"

#include "Engine/Engine.h"

#include "Engine/Controllers/CameraController.h"

#include "Components/AnimObject.h"

SimpleScene::SimpleScene(Engine* engine) : Scene(engine)
{
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
		0.610f, 1.f, 0.02f, 0.8f, { 10,20,0 }, Vec3(-1, -1, -1).Normalize(), { 1, 1, 1 });

	engine->Renderer()->LightSystem()->AddLight(sunLight);
	engine->Renderer()->LightSystem()->AddShadow(sunLight, SHADOW_MAP_QUALITY::HIGH);
	//follow cam
	engine->Renderer()->LightSystem()
		->GetLightExData<LightSystem::ExtraDataCSMDirLight>(sunLight)
		->Follow(
			&engine->Renderer()->GetTargetCamera()->ViewMatrix(), 
			&engine->Renderer()->GetTargetCamera()->ProjectionMatrix()
		);

	auto lightNode = NewLightNode();
	lightNode->RenderingObject().lightID = sunLight;
	lightNode->Blob().AsLight() = engine->Renderer()->LightSystem()->GetLight(sunLight);
	m_nodes.push_back(lightNode);


	auto cameraNode = NewCameraNode();
	cameraNode->Deleter() = camDeleter;
	//cameraNode->StateChange() = 0;
	cameraNode->Controller() = new CameraController(
		{ 0, 50, 0 }, { 0,50,50 }, ConvertToRadians(60), 0.5, 1000,
		engine->Renderer()->GetRenderWidth() / (float)engine->Renderer()->GetRenderHeight()
	);
	cameraNode->RenderingObject().camera = engine->m_cam;
	m_nodes.push_back(cameraNode);


	//============================================================================================
	auto node = NewRenderableObjectNode();
	node->Deleter() = deleter;
	IRenderableObject* obj = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Copper.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj->Transform() *= GetScaleMatrix(1, 5, 0.5f);
	obj->Transform().SetPosition(0, 0, 10);
	node->RenderingObject().renderableObject = obj;
	node->Transform() = obj->Transform();
	m_nodes.push_back(node);


	//============================================================================================
	node = NewRenderableObjectNode();
	node->Deleter() = deleter;
	obj = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/sphere.obj",
		L"D:/KEngine/ResourceFile/temp_img/Blue.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj->SetPosition(15, 30, -20);
	node->RenderingObject().renderableObject = obj;
	node->Transform() = obj->Transform();
	m_nodes.push_back(node);


	//============================================================================================
	node = NewRenderableObjectNode();
	node->Deleter() = deleter;
	obj = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Copper.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj->Transform().SetScale(1, 5, 0.5f);
	obj->Transform().SetPosition(0, 0, 20);
	node->RenderingObject().renderableObject = obj;
	node->Transform() = obj->Transform();
	m_nodes.push_back(node);


	//============================================================================================
	node = NewRenderableObjectNode();
	node->Deleter() = deleter;
	obj = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Copper.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj->Transform().SetScale(1, 5, 10.f);
	obj->Transform().SetPosition(0, 0, -30);
	node->RenderingObject().renderableObject = obj;
	node->Transform() = obj->Transform();
	m_nodes.push_back(node);


	//============================================================================================
	node = NewRenderableObjectNode();
	node->Deleter() = deleter;
	obj = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Copper.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj->Transform().SetScale(1, 5, 10.f);
	obj->Transform().SetPosition(0, 0, 60);
	node->RenderingObject().renderableObject = obj;
	node->Transform() = obj->Transform();
	m_nodes.push_back(node);


	//============================================================================================
	node = NewRenderableObjectNode();
	node->Deleter() = deleter;
	obj = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Blue.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj->Transform().SetScale(0.5f, 10, 32.0f);
	obj->Transform().SetPosition(0, 40, 0);
	node->RenderingObject().renderableObject = obj;
	node->Transform() = obj->Transform();
	m_nodes.push_back(node);


	//============================================================================================
	node = NewRenderableObjectNode();
	node->Deleter() = deleter;
	obj = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Blue.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj->Transform().SetScale(0.5f, 3, 10.0f);
	obj->Transform().SetPosition(0, -10, 15);
	node->RenderingObject().renderableObject = obj;
	node->Transform() = obj->Transform();
	m_nodes.push_back(node);


	//============================================================================================
	node = NewRenderableObjectNode();
	node->Deleter() = deleter;
	obj = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Blue.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj->Transform().SetScale(0.5f, 6, 20.0f);
	obj->Transform() *= GetRotationAxisMatrix({ 0, 1, 0 }, PI / 2);
	obj->Transform().SetPosition(-45, 0, 50);
	node->RenderingObject().renderableObject = obj;
	node->Transform() = obj->Transform();
	m_nodes.push_back(node);


	//============================================================================================
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


	//============================================================================================
	node = NewRenderableObjectNode();
	node->Deleter() = deleter;

	std::vector<PBRMaterialPath> paths = {
			{ L"D:/KEngine/ResourceFile/model/character/Character Texture 256x256.png", L"", L"", L"", L"" }
	};
	auto animObject = new PBRMultiMeshAnimObject(
		L"D:/KEngine/ResourceFile/model/character/Character1.fbx",
		paths,
		GetScaleMatrix(0.02f, 0.02f, 0.02f)
	);
	//animObject->Animator().SetAnimation(0);

	obj = animObject;
	/*obj = new BasicObject(
		L"D:/KEngine/ResourceFile/model/character/Character Running.fbx",
		L"D:/KEngine/ResourceFile/model/character/Character Texture 256x256.png",
		GetScaleMatrix(0.02f, 0.02f, 0.02f)
	);*/
	obj->Transform() *= GetRotationYMatrix(PI / 2.0f);
	obj->Transform().SetPosition(50, 0, 30);
	node->RenderingObject().renderableObject = obj;
	node->Transform() = obj->Transform();
	m_nodes.push_back(node);


	for (auto& node : m_nodes)
	{
		if (node->Type() == SceneNode::RENDERABLE_NODE)
			node->m_aabb = node->RenderingObject().renderableObject->GetAABB();
	}

	EndUpdate(0);
}

SimpleScene::~SimpleScene()
{
	BeginUpdate(0);

	for (auto& node : m_nodes)
	{
		delete node;
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