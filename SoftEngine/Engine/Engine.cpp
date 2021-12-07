#include "Engine.h"

#include "Global.h"

#include <iostream>

#include <Common.h>

#include <PrimitiveTopology.h>
#include <Renderer.h>

#include <Math/Math.h>
#include <Math/Collision.h>

#include <Resource.h>
#include <RenderPipeline.h>

#include <ILightSystem.h>

#include <Component/FPPCamera.h>
#include <Component/SpaceCoordinate.h>
#include <Component/SkyBox.h>

#include <Component/StaticObject.h>
#include <Component/Example/Asteroid.h>

//#include "./DX11/LightSystem_v2.h"

#include <Component/HeightMap.h>
#include <Component/Water.h>

#include <Component/AnimObject.h>

#include <Component/Frustum.h>

ICamera* cam = nullptr;

SpaceCoordinate* spaceCoord = nullptr;

IRenderableObject* skyBox = nullptr;

IRenderableObject* basicObject = nullptr;

IRenderableObject* basicObject1 = nullptr;

IRenderableObject* obj1 = nullptr;
IRenderableObject* obj2 = nullptr;

IRenderableObject* wall1 = nullptr;
IRenderableObject* wall2 = nullptr;
IRenderableObject* wall3 = nullptr;
IRenderableObject* wall4 = nullptr;

IRenderableObject* wall5 = nullptr;

IInstancingObject* instacingObj1 = nullptr;

IRenderableObject* obj3 = nullptr;

IRenderableObject* obj4 = nullptr;

IRenderableObject* obj5 = nullptr;

IRenderableObject* obj6 = nullptr;

LightID shadowLight1;
LightID shadowLight2;

LightID lights[6] = {};

std::vector<IRenderableObject*> renderList;


IRenderableObject* pbrModelTest = nullptr;

IRenderableObject* heightMap = nullptr;

IRenderableObject* gerstnerWater = nullptr;

PBRMultiMeshAnimObject* animModelObj = nullptr;

IRenderableObject* lightObj1 = nullptr;

//float radius = 0;
//Vec3 center = {};

std::vector<float> radiuses;
std::vector<Vec3> centers;
std::vector<float> depthThres;

void GetRandomSpace(const Vec3& dir, Vec3* outTangent, Vec3* outBitangent)
{
	auto& tangent = *outTangent;
	auto& bitangent = *outBitangent;

	Vec3 c1 = CrossProduct(dir, Vec3(0.0, 0.0, 1.0));
	Vec3 c2 = CrossProduct(dir, Vec3(0.0, 1.0, 0.0));

	if (c1.Length() > c2.Length())
	{
		tangent = c1;
	}
	else
	{
		tangent = c2;
	}

	tangent.Normalize();

	bitangent = CrossProduct(tangent, dir);
	bitangent.Normalize();
}

void CalBoundingSphere(std::vector<Vec3>& corners, Vec3* outCenter, float* outRadius)
{
	//0-----1
	//|     |
	//3-----2
	auto* farPlane = &corners[4];
	auto* nearPlane = &corners[0];

	auto& p1 = nearPlane[0];
	auto& p2 = nearPlane[2];

	auto& p3 = farPlane[0];
	auto& p4 = farPlane[2];

	auto c1 = (p1 + p3) / 2.0f;
	auto c2 = (p3 + p4) / 2.0f;

	Plane3D plane1 = Plane3D(c1, p3 - p1);
	Plane3D plane2 = Plane3D(c2, p4 - p3);

	//plane contains p1, p2, p3
	Plane3D plane3 = Plane3D(p1, p2, p3);

	//must contains p4
	assert(IsFloatEqual(plane3.Value(p4), 0, 0.001f));

	auto line = plane1.Intersect(plane2);

	auto center = line.Intersect(plane3);

	auto radius = (center - p1).Length();
	*outCenter = center;
	*outRadius = radius;
}

//void CalBoundingSphere(std::vector<Vec3>& corners, Vec3* outCenter, float* outRadius)
//{
//	//0-----1
//	//|     |
//	//3-----2
//	auto* farPlane = &corners[4];
//	auto* nearPlane = &corners[0];
//	
//	Vec3 center = {};
//	for (size_t i = 0; i < 8; i++)
//	{
//		center = center + corners[i];
//	}
//	center = center / 8.0f;
//
//	float r = 0;
//	for (size_t i = 0; i < 8; i++)
//	{
//		r = max((center - corners[i]).Length(), r);
//	}
//
//	*outRadius = r;
//	*outCenter = center;
//}

Mat4x4 Tie(const Vec3& dir, const Mat4x4& _view, const Vec3& center, float radius)
{
	auto centerInWolrdSpace = ConvertVector(Vec4(center, 1.0f) * _view);

	const float nearOffset = 1000.f;

	Mat4x4 proj;
	proj.SetOrthographicLH(radius * 2.0f, radius * 2.0f, 0, radius * 2.0f + nearOffset);
	
	Mat4x4 view;
	auto pos = centerInWolrdSpace - dir.Normal() * (radius + nearOffset);

	view.SetLookAtLH(pos, centerInWolrdSpace, { 0,1,0 });

	auto vp = view * proj;

	const float factor = 1024.0f / 2;

	//origin must be on a line on pixel grid
	Vec4 shadowOrigin = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
	shadowOrigin = shadowOrigin * vp;
	shadowOrigin = shadowOrigin * factor;

	Vec4 roundedOrigin = Vec4(std::floor(shadowOrigin.x), std::floor(shadowOrigin.y), 0, 0);
	Vec4 roundOffset = roundedOrigin - shadowOrigin;
	roundOffset = roundOffset / factor;

	proj.SetPosition(roundOffset.x, roundOffset.y, 0);

	return view * proj;
}

void SeparateFrustum(const Mat4x4& proj, std::vector<Vec3>& corners, std::vector<std::vector<Vec3>>& frustums)
{
	//std::vector<Vec3> corners;
	//Frustum::GetFrustumCorners(corners, cam->ProjectionMatrix());

	auto* farPlane = &corners[4];
	auto* nearPlane = &corners[0];

	Vec3 dir[4] = {};

	for (size_t i = 0; i < 4; i++)
	{
		dir[i] = (farPlane[i] - nearPlane[i]).Normalize();
	}

	float thres[ShadowMap_NUM_CASCADE] = {
		50,
		100,
		150,
		150
	};

	float totalLength = thres[0] + thres[1] + thres[2] + thres[3];
	float expectLength = (farPlane[0] - nearPlane[0]).Length();

	for (size_t i = 0; i < ShadowMap_NUM_CASCADE; i++)
	{
		thres[i] = (thres[i] / totalLength) * expectLength;
	}

	float count = 0;
	for (size_t i = 0; i < ShadowMap_NUM_CASCADE; i++)
	{
		auto c = count + thres[i];

		std::vector<Vec3> temp =  {
			//near plane
			nearPlane[0] + dir[0] * count, 
			nearPlane[1] + dir[1] * count,
			nearPlane[2] + dir[2] * count,
			nearPlane[3] + dir[3] * count,

			//far plane
			nearPlane[0] + dir[0] * c, 
			nearPlane[1] + dir[1] * c, 
			nearPlane[2] + dir[2] * c, 
			nearPlane[3] + dir[3] * c
		};

		auto farPlaneCenter = (temp[4] + temp[5] + temp[6] + temp[7]) / 4.0f;
		auto v = Vec4(farPlaneCenter, 1.0f) * proj;
		v = v / v.w;

		depthThres.push_back(v.z);

		frustums.push_back(temp);

		count = c;		
	}
	
}

void CalBoundingSphere(ICamera* cam)
{
	std::vector<Vec3> corners;
	Frustum::GetFrustumCorners(corners, cam->ProjectionMatrix());

	std::vector<std::vector<Vec3>> frustums;
	SeparateFrustum(cam->ProjectionMatrix(), corners, frustums);

	for (size_t i = 0; i < frustums.size(); i++)
	{
		float r = 0;
		Vec3 center;

		CalBoundingSphere(frustums[i], &center, &r);

		centers.push_back(center);
		radiuses.push_back(r);
	}
}

void TieProjShadow(class LightSystem* lightSys, ICamera* cam)
{
	auto& light = lightSys->GetLight(shadowLight1);
	auto shadowProj = lightSys->GetShadow(shadowLight1);
	auto camProj = cam->ProjectionMatrix();
	auto camView = cam->ViewMatrix();

	for (size_t i = 0; i < centers.size(); i++)
	{
		shadowProj[i] = Tie(light.dir, camView, centers[i], radiuses[i]);
	}

	float* p = (float*)&shadowProj[ShadowMap_NUM_CASCADE];
	for (size_t i = 0; i < depthThres.size(); i++)
	{
		p[i] = depthThres[i];
	}

	lightSys->ForceUpdateShadow(0);
}

Engine::Engine(const wchar_t* title, int width, int height) : Window(title, width, height)
{
	//SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	//::SetProcessDPIAware();

	Random::Initialize();

	m_currentTime = GetTime();

	m_resourceManager = new Resource();

	m_rplManager = new class RenderPipelineManager();

	Global::engine = this;
	Global::input = Input();
	Global::resourceManager = ResourceManager();
	Global::rplManager = RenderPipelineManager();


	void* args[] = { GetNativeHandle() };
	m_renderer = new DeferredRenderer(2, width, height, 1, args);
	
	Global::renderer = Renderer();

	cam = new FPPCamera({ 0, 50, 0 }, { 0,50,50 }, ConvertToRadians(75), 1, 1000, width / (float)height);
	m_renderer->SetTargetCamera(cam);

	CalBoundingSphere(cam);
	
	spaceCoord = new SpaceCoordinate();

	spaceCoord->DisplayGrid() = false;

	skyBox = new SkyCube(L"D:/KEngine/ResourceFile/temp_img/Skybox/skybox1_low.png");
	m_renderer->AttachSkyBox(skyBox);

	//====================init scene object=================================================

	shadowLight1 = m_renderer->LightSystem()->NewLight(LIGHT_TYPE::CSM_DIRECTIONAL_LIGHT, 0,
		0.1f, 0.1f, 0.1f, 0.7f, { 10,20,0 }, { 0, -1, -1 }, { 1, 1, 1 });

	m_renderer->LightSystem()->AddLight(shadowLight1);
	m_renderer->LightSystem()->AddShadow(shadowLight1, SHADOW_MAP_QUALITY::HIGH);
	//m_renderer->LightSystem()->AddShadowEx(shadowLight1, )

	m_renderer->LightSystem()->Log();

	obj1 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Copper.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj1->Transform() *= GetScaleMatrix(1, 1, 1);
	obj1->Transform().SetPosition(0, 0, 10);

	obj2 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/sphere.obj",
		L"D:/KEngine/ResourceFile/temp_img/Blue.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj2->SetPosition(0, 0, -10);

	obj3 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Copper.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj3->Transform() *= GetRotationAxisMatrix({ 1, 1, 1 }, PI / 4);
	obj3->Transform().SetPosition(45, 20, 0);


	obj4 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/SuzanneSmooth.obj",
		L"D:/KEngine/ResourceFile/temp_img/Copper.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj4->Transform() *= GetRotationAxisMatrix({ 1, 1, 1 }, PI / 4);
	obj4->Transform().SetPosition(-45, 20, 0);


	obj5 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/teapot.obj",
		L"D:/KEngine/ResourceFile/temp_img/Copper.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj5->Transform() *= GetRotationAxisMatrix({ 1, 1, 1 }, PI / 4);
	obj5->Transform().SetPosition(0, 20, 45);

	obj6 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/teapot.obj",
		L"D:/KEngine/ResourceFile/temp_img/Blue.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj6->Transform() *= GetRotationAxisMatrix({ 1, 1, 1 }, PI / 4);
	obj6->Transform().SetPosition(0, 20, -45);

	//objs make shadow
	renderList.push_back(obj1);
	renderList.push_back(obj2);
	renderList.push_back(obj3);

	renderList.push_back(obj4);
	renderList.push_back(obj5);
	renderList.push_back(obj6);


	float unit = 0.8f;

	heightMap = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/Plane.obj",
		L"D:/KEngine/ResourceFile/temp_img/white.png",
		GetScaleMatrix(2000, 2000, 2000)
	);
	heightMap->Transform().SetTranslation(0, -15, 0);

	unit = 1.f;
	size_t dimX = 64;
	gerstnerWater = new GerstnerWavesWater(dimX, dimX, 15, unit, unit);
	gerstnerWater->SetPosition(-unit * dimX * 0.5f, -10, -unit * dimX * 0.5f);

	/*std::vector<PBRMaterialPath> paths = {
		{
			L"D:/KEngine/ResourceFile/model/character/globin/textures/lowRes/character diffuse.png",
			L"D:/KEngine/ResourceFile/model/character/globin/textures/lowRes/character normals.png",
			L"D:/KEngine/ResourceFile/model/character/globin/textures/lowRes/character specular.png",
			L"",
			L"",
		},
		{
			L"D:/KEngine/ResourceFile/model/character/globin/textures/lowRes/weapons diffuse.png",
			L"D:/KEngine/ResourceFile/model/character/globin/textures/lowRes/weapons normals.png",
			L"D:/KEngine/ResourceFile/model/character/globin/textures/lowRes/weapons specular.png",
			L"",
			L""
		},
		{
			L"D:/KEngine/ResourceFile/model/character/globin/textures/lowRes/character diffuse.png",
			L"D:/KEngine/ResourceFile/model/character/globin/textures/lowRes/character normals.png",
			L"D:/KEngine/ResourceFile/model/character/globin/textures/lowRes/character specular.png",
			L"",
			L"",

		},
	};
	animModelObj = new PBRMultiMeshAnimObject(L"D:/KEngine/ResourceFile/model/character/globin/globin.fbx", 
		paths, GetScaleMatrix(0.1, 0.1, 0.1));
	animModelObj->Animator().SetAnimation(11);
	animModelObj->Animator().Reset();*/


	lightObj1 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/white.png"
	);
	auto* temp = RenderPipelineManager::Get(
		"",
		L"Model3D/VSBasicModel",
		L"Light/PS_NonTBN_LightVisualize"
	);
	lightObj1->SetRenderPipeline(temp);
	RenderPipelineManager::Release(&temp);

	lightObj1->Transform().SetScale(0.5, 0.5, 0.5);
	lightObj1->Transform().SetPosition({ 10,20,0 });


	for (size_t i = 0; i < 100; i++)
	{
		renderList.push_back(new BasicObject(
			L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
			L"D:/KEngine/ResourceFile/temp_img/Copper.png"
		));
		auto& back = renderList.back();

		auto pos = Vec3(Random::Float(-100, 100), 0, Random::Float(-1000, 1000));
		auto rot = Vec3(Random::Float(-PI, PI), Random::Float(-PI, PI), Random::Float(-PI, PI));

		back->Transform() *= GetRotationMatrix(rot.x, rot.y, rot.z);
		back->Transform().SetPosition(pos);

	}

}

Engine::~Engine()
{
	delete cam;
	delete spaceCoord;
	delete skyBox;
	delete basicObject;
	delete basicObject1;
	/*delete obj1;
	delete obj2;
	delete obj3;
	delete obj4;
	delete obj5;
	delete obj6;*/

	for (size_t i = 0; i < renderList.size(); i++)
	{
		delete renderList[i];
	}

	delete wall1;
	delete wall2;
	delete wall3;
	delete wall4;
	delete wall5;
	delete instacingObj1;

	delete pbrModelTest;

	delete heightMap;
	delete gerstnerWater;

	delete animModelObj;

	delete lightObj1;

	TBNAnimModel::FreeStaticResource();

	delete m_renderer;

	delete m_rplManager;

	delete m_resourceManager;

	Random::UnInitialize();
}


void Engine::Update()
{
	auto cur = GetTime();
	m_deltaTime = cur - m_currentTime;
	m_currentTime = cur;

	m_time += m_deltaTime / _TIME_FACTOR;

	Sleep(2);
	
	//do update
	//auto d = PI / 5 * FDeltaTime();

	static bool lock = true;
	static int visualizeArg = 0;

	if(!lock) cam->Update(this);

	if (Input()->GetPressKey(TAB))
	{
		lock = !lock;
		Input()->SetLockMouse(!lock, 500, 200);
		Input()->SetHideCursor(!lock);
	}

	if (Input()->GetPressKey('1'))
	{
		visualizeArg = 0;
	}
	if (Input()->GetPressKey('2'))
	{
		visualizeArg = 1;
	}
	if (Input()->GetPressKey('3'))
	{
		visualizeArg = 2;
	}
	/*if (Input()->GetPressKey('4'))
	{
		visualizeArg = 3;
	}*/

	gerstnerWater->Update(this);

	static size_t currentAnim = 0;
	if (Input()->GetPressKey('M'))
	{
		currentAnim = (currentAnim + 1) % animModelObj->Animator().AnimationCount();
		animModelObj->Animator().SetAnimation(currentAnim);
		animModelObj->Animator().Reset(true);
	}

	if (Input()->GetPressKey('N'))
	{
		if (currentAnim == 0) currentAnim = animModelObj->Animator().AnimationCount() - 1;
		else
			currentAnim = (currentAnim - 1) % animModelObj->Animator().AnimationCount();
		animModelObj->Animator().SetAnimation(currentAnim);
		animModelObj->Animator().Reset(true);
	}

	if (Input()->GetPressKey('X'))
	{
		float cDuration = animModelObj->Animator().Duration();
		animModelObj->Animator().SetDuration(cDuration + 1);
	}
	if (Input()->GetPressKey('Z'))
	{
		float cDuration = animModelObj->Animator().Duration();
		animModelObj->Animator().SetDuration(cDuration - 1);
	}

	if (Input()->GetPressKey(UP_ARROW))
	{
		auto& light = m_renderer->LightSystem()->GetLight(shadowLight1);
		light.power += 0.01f;
		m_renderer->LightSystem()->UpdateLight(shadowLight1);
	}
	if (Input()->GetPressKey(DOWN_ARROW))
	{
		auto& light = m_renderer->LightSystem()->GetLight(shadowLight1);
		light.power -= 0.01f;
		m_renderer->LightSystem()->UpdateLight(shadowLight1);
	}

	//animModelObj->Update(this);

	TieProjShadow(m_renderer->LightSystem(), m_renderer->GetTargetCamera());

	//must be the last update
	m_renderer->LightSystem()->Update();

	static float clsCol[] = { 0,0,0,0 };
	m_renderer->ClearFrame(clsCol);

	Render();

#ifdef _DEBUG
	if (visualizeArg < 2)
	{
		m_renderer->VisualizeBackgroundRenderPipeline(visualizeArg);
	}
	else
		m_renderer->Present();
#else
	m_renderer->Present();
#endif // _DEBUG
}

void Engine::Render()
{
	//basicObject->Render(m_renderer);

	for (auto& obj : renderList)
	{
		obj->Render(m_renderer);
	}

	//pbrModelTest->Render(m_renderer);

	//animModelObj->Render(m_renderer);

	heightMap->Render(m_renderer);

	spaceCoord->Render(m_renderer);

	lightObj1->Render(m_renderer);
	
	if (m_renderer->LightSystem()->BeginShadow(shadowLight1))
	{
		for (auto& obj : renderList)
		{
			obj->Render(m_renderer);
		}
		m_renderer->LightSystem()->EndShadow(shadowLight1);
	}

	m_renderer->BeginTransparency();
	gerstnerWater->Render(m_renderer);
	m_renderer->EndTransparency();
	
}