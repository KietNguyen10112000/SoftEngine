#include "Engine.h"

#include "Global.h"

#include <iostream>

#include <Common.h>

#include <PrimitiveTopology.h>
#include <Renderer.h>

#include <Math/Math.h>

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

#include <PostProcessor.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

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


WNDPROC g_oldHWNDHandle = 0;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT WndHandle2(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	if (g_oldHWNDHandle) return g_oldHWNDHandle(hWnd, uMsg, wParam, lParam);

	return false;
}

void InitImgui(Window* window)
{
	auto hwnd = window->GetNativeHandle();
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(DX11Global::renderer->m_d3dDevice, DX11Global::renderer->m_d3dDeviceContext);

	constexpr int GWL_WNDPROC_ = -4;
	g_oldHWNDHandle = (WNDPROC)SetWindowLongPtr(hwnd, GWL_WNDPROC_, (LONG_PTR)WndHandle2);
}

void DestroyImgui()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
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

	cam = new FPPCamera({ 0, 50, 0 }, { 0,50,50 }, ConvertToRadians(60), 0.5, 1000, width / (float)height);
	m_renderer->SetTargetCamera(cam);

	shadowLight1 = m_renderer->LightSystem()->NewLight(LIGHT_TYPE::POINT_LIGHT, 0,
		0.1f, 1.69f, 0.02f, 20.f, { 10,20,0 }, { -1, -1, -1 }, { 1, 1, 1 });

	shadowLight2 = m_renderer->LightSystem()->NewLight(LIGHT_TYPE::SPOT_LIGHT, ConvertToRadians(60),
		0.1f, 0.1f, 0.04f, 40.f, { 30,30,30 }, { -1, -1, -1 }, { 1, 1, 1 });

	//m_renderer->LightSystem()->AddLight(shadowLight1);
	m_renderer->LightSystem()->AddLight(shadowLight2);

	m_renderer->LightSystem()->AddShadow(shadowLight2, SHADOW_MAP_QUALITY::HIGH);
	//m_renderer->LightSystem()->AddShadow(shadowLight1, SHADOW_MAP_QUALITY::HIGH);

	m_renderer->LightSystem()->Log();

	spaceCoord = new SpaceCoordinate();

	spaceCoord->DisplayGrid() = false;

	skyBox = new SkyCube(L"D:/KEngine/ResourceFile/temp_img/skybox-blue-night-sky.png");
	m_renderer->AttachSkyBox(skyBox);

	obj1 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Copper.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj1->Transform() *= GetScaleMatrix(1, 5, 0.5f);
	obj1->Transform().SetPosition(0, 0, 10);

	obj2 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/sphere.obj",
		L"D:/KEngine/ResourceFile/temp_img/Blue.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj2->SetPosition(0, 0, -20);

	obj3 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Copper.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj3->Transform().SetScale(1, 5, 0.5f);
	//obj3->Transform() *= GetRotationAxisMatrix({ 1, 1, 1 }, PI / 4);
	obj3->Transform().SetPosition(0, 0, 20);


	obj4 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Copper.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj4->Transform().SetScale(1, 5, 0.5f);
	//obj4->Transform() *= GetRotationAxisMatrix({ 1, 1, 1 }, PI / 4);
	obj4->Transform().SetPosition(0, 0, 0);


	obj5 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Copper.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj5->Transform().SetScale(1, 5, 10.f);
	//obj5->Transform() *= GetRotationAxisMatrix({ 1, 1, 1 }, PI / 4);
	obj5->Transform().SetPosition(0, 0, 60);

	obj6 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/simple_model/cube1.obj",
		L"D:/KEngine/ResourceFile/temp_img/Blue.png",
		GetScaleMatrix(3, 3, 3)
	);
	obj6->Transform().SetScale(0.5f, 3, 10.0f);
	//obj6->Transform() *= GetRotationAxisMatrix({ 1, 0, 0 }, PI / 2);
	obj6->Transform().SetPosition(0, 20, 15);


	//5 wall
	float d = 100;
	float h = 50;

	wall1 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/PBRModel test/Wall.obj",
		L"D:/KEngine/ResourceFile/temp_img/white.png",
		GetScaleMatrix(2.f, 50.f, 50.f)
	);
	wall1->SetPosition(d, h, 0);

	wall2 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/PBRModel test/Wall.obj",
		L"D:/KEngine/ResourceFile/temp_img/white.png",
		GetScaleMatrix(5.f, 5.f, 5.f)
	);
	wall2->SetPosition(-d, h, 0);

	wall3 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/PBRModel test/Wall.obj",
		L"D:/KEngine/ResourceFile/temp_img/white.png",
		GetScaleMatrix(5.f, 5.f, 5.f)
	);
	wall3->Transform().SetRotationY(PI / 2).SetPosition(0, h, d);

	wall4 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/PBRModel test/Wall.obj",
		L"D:/KEngine/ResourceFile/temp_img/white.png",
		GetScaleMatrix(5.f, 5.f, 5.f)
	);
	wall4->Transform().SetRotationY(PI / 2).SetPosition(0, h, -d);

	wall5 = new BasicObject(
		L"D:/KEngine/ResourceFile/model/PBRModel test/Wall.obj",
		L"D:/KEngine/ResourceFile/temp_img/white.png",
		GetScaleMatrix(5.f, 5.f, 5.f)
	);
	wall5->Transform().SetRotationZ(PI / 2).SetPosition(0, d + h, 0);


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
		GetScaleMatrix(1000, 1000, 1000)
	);
	heightMap->Transform().SetTranslation(0, -15, 0);

	/*lightObj1 = new BasicObject(
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
	lightObj1->Transform().SetPosition({ 10,20,0 });*/

	

	//=================================test post processing======================================
	auto postproc = m_renderer->PostProcessor();
	/*auto position = postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::POSITION_AND_SPECULAR);
	auto normal = postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::NORMAL_AND_SHININESS);
	auto color = postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::COLOR);*/
	
	bloomSV = new ShaderVar(&bloomData, sizeof(bloomData));

	constexpr static float DOWN_RES_FACTOR = 1.0f;

	auto lastscene = postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::LIGHTED_SCENE);
	auto screenSurface = postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::SCREEN_BUFFER);

	auto output1 = postproc->GetTexture2D(Vec2(m_width / DOWN_RES_FACTOR, m_height / DOWN_RES_FACTOR));
	auto layer2 = postproc->MakeLayer({ output1 });

	auto gausianBlurPS = Resource::Get<PixelShader>(L"PostProcessing/Blur/GaussianBlur");
	auto combinePS = Resource::Get<PixelShader>(L"PostProcessing/Blur/PSCombine");

	struct Temp
	{
		ShaderVar* buf;
		float w;
		float h;
	};

	static Temp _opaque = {};
	_opaque.buf = bloomSV;
	_opaque.w = m_width;
	_opaque.h = m_height;

	//god rays post program
	auto godRays = postproc->MakeProgram("God Rays");
	auto godRayOutput0 = postproc->GetTexture2D(Vec2(m_width / DOWN_RES_FACTOR, m_height / DOWN_RES_FACTOR));

	auto scenePosition = postproc->GetTexture2D(PostProcessor::AvaiableTexture2D::POSITION_AND_SPECULAR);

	auto godRaysInputLayer = postproc->MakeLayer({ scenePosition });

	auto godRaysLayer1 = postproc->MakeLayer({ godRayOutput0 , lastscene });

	auto screenLayer = postproc->MakeLayer({ screenSurface });

	auto godRaysPS = Resource::Get<PixelShader>(L"PostProcessing/LightEffect/GodRays/GodRays_PS");
	auto godRaysCombinePS = Resource::Get<PixelShader>(L"PostProcessing/LightEffect/GodRays/Combine");

	//auto biBlurPS = Resource::Get<PixelShader>(L"PostProcessing/Blur/BilateralBlur");

	if (!godRays->IsCrafted())
	{
		godRays->Append(godRaysInputLayer);

		godRays->Append(godRaysPS, &_opaque,
			[](IRenderer*, void* opaque)
			{
				ShaderVar* buf = _opaque.buf;
				bloomData.gaussian.type = 0;
				bloomData.gaussian.width = _opaque.w / DOWN_RES_FACTOR;
				bloomData.gaussian.height = _opaque.h / DOWN_RES_FACTOR;
				bloomData.gaussian.downscale = DOWN_RES_FACTOR;
				buf->Update(&bloomData, sizeof(bloomData));
				RenderPipeline::PSSetVar(buf, 2);
			}
		);

		//for (size_t i = 0; i < 5; i++)
		//{
		godRays->Append(godRaysLayer1);
		//=================2 pass Gaussian Blur=============================
		godRays->Append(gausianBlurPS, &_opaque,
			[](IRenderer*, void* opaque)
			{
				ShaderVar* buf = _opaque.buf;
				bloomData.gaussian.type = 0;
				bloomData.gaussian.width = _opaque.w / DOWN_RES_FACTOR;
				bloomData.gaussian.height = _opaque.h / DOWN_RES_FACTOR;
				buf->Update(&bloomData, sizeof(bloomData));
			}
		);

		godRays->Append(layer2);

		godRays->Append(gausianBlurPS, &_opaque,
			[](IRenderer* renderer, void* opaque)
			{
				ShaderVar* buf = _opaque.buf;
				bloomData.gaussian.type = 1;
				buf->Update(&bloomData, sizeof(bloomData));
			}
		);
		//=================End 2 pass Gaussian Blur==========================
		//}

		godRays->Append(godRaysLayer1);

		godRays->Append(godRaysCombinePS);
		godRays->Append(screenLayer);

		auto ret = godRays->Craft();
		if (ret != "OK")
		{
			exit(2);
		}
	}

	auto postprocChain = postproc->MakeProcessChain("Bloom Effect");

	if (!postprocChain->IsCrafted())
	{	
		postprocChain->Append(
			{
				// programs
				{
					//re-run
					{ godRays, true }
				},

				{
					{
						{ {0, 0}, {0, 0} }
					}
				}
			}
		);

		auto ret = postprocChain->Craft();
		/*if (ret != "OK")
		{
			exit(2);
		}*/
	}
	

	postproc->SetProcessChain(postprocChain);

	Resource::Release(&gausianBlurPS);
	//Resource::Release(&lumaPS);
	Resource::Release(&combinePS);

	Resource::Release(&godRaysPS);
	Resource::Release(&godRaysCombinePS);

	InitImgui(this);
}

Engine::~Engine()
{
	DestroyImgui();

	delete bloomSV;

	delete cam;
	delete spaceCoord;
	delete skyBox;
	delete basicObject;
	delete basicObject1;
	delete obj1;
	delete obj2;
	delete obj3;
	delete obj4;
	delete obj5;
	delete obj6;
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

void ImGuiShowLight(IRenderer* renderer, LightID id)
{
	static float lowspeed = 0.01f;
	static float midspeed = 0.1f;
	static float highspeed = 1.f;
	static float high1speed = 5.f;

	auto& light = renderer->LightSystem()->GetLight(id);

	static Vec3 tempDir = light.dir;

	bool change = false;

	ImGui::Begin(("Light " + std::to_string(id)).c_str());

	const char* text = 0;
	switch (light.type)
	{
	case 0:
		text = "Direction Light";
		break;
	case 1:
		text = "Point Light";
		break;
	case 2:
		text = "Spot Light";
		break;
	default:
		break;
	}
	ImGui::Text(text);

	change |= ImGui::DragFloat3("Position", &light.pos.x, midspeed, -FLT_MAX, FLT_MAX);
	change |= ImGui::DragFloat3("Direction", &tempDir.x, lowspeed, -FLT_MAX, FLT_MAX);

	change |= ImGui::DragFloat3("Color", &light.color.x, lowspeed, 0, 1);
	change |= ImGui::DragFloat("Power", &light.power, highspeed, 1, FLT_MAX);
	change |= ImGui::DragFloat("Spot Angle", &light.spotAngle, lowspeed, 0, PI);
	
	change |= ImGui::DragFloat("Constant Attenuation", &light.constantAttenuation, lowspeed, 0.0001f, FLT_MAX);
	change |= ImGui::DragFloat("Linear Attenuation", &light.linearAttenuation, lowspeed, 0.0001f, FLT_MAX);
	change |= ImGui::DragFloat("Quadratic Attenuation", &light.quadraticAttenuation, lowspeed, 0.0001f, FLT_MAX);

	ImGui::End();

	if (change)
	{
		light.dir = tempDir.Normal();
		renderer->LightSystem()->UpdateLight(id);
		renderer->LightSystem()->UpdateShadow(id);
	}
}

void ImGuiShowLightSystemInfo(IRenderer* renderer)
{
	static float lowspeed = 0.001f;

	auto& info = renderer->LightSystem()->m_info;

	bool change = false;

	ImGui::Begin("Light System Info");
	change |= ImGui::DragFloat3("Env ambient", &info.environmentAmbient.x, lowspeed, -FLT_MAX, FLT_MAX);
	change |= ImGui::DragFloat3("Offset pixel light", &info.offsetPixelLightFactor.x, lowspeed, -FLT_MAX, FLT_MAX);
	change |= ImGui::DragFloat("Depth bias", &info.depthBias, 0.0000001, -FLT_MAX, FLT_MAX, "%.6f");
	ImGui::End();

	if (change)
	{
		renderer->LightSystem()->UpdateEnv(info);
	}
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

	if (!lock) cam->Update(this);

	static int rotDir = 0;

	if (Input()->GetPressKey(LEFT_ARROW))
	{
		rotDir++;
	}
	if (Input()->GetPressKey(RIGHT_ARROW))
	{
		rotDir--;
	}

	if (rotDir > 0)
	{
		Mat4x4 temp;
		for (size_t i = 0; i < 6; i++)
		{
			auto& light = m_renderer->LightSystem()->GetLight(lights[i]);
			temp.SetPosition(light.pos);
			temp *= GetRotationYMatrix(FDeltaTime() * (PI / 6));
			light.pos = temp.GetPosition();

			m_renderer->LightSystem()->UpdateLight(lights[i]);
			//m_renderer->LightSystem()->UpdateShadow(lights[i]);
		}
	}

	if (Input()->GetPressKey(TAB))
	{
		lock = !lock;
		Input()->SetLockMouse(!lock, 500, 200);
		Input()->SetHideCursor(!lock);
	}

	if (Input()->GetPressKey('Q'))
	{
		visualizeArg = (visualizeArg + 1) % 3;
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

	static bool moveLight1 = false;
	static float light1Timer = 0;
	static bool rotateLight2 = false;
	static bool moveLight1WithCam = false;
	//static float counter = 0;

	if (Input()->GetPressKey('H'))
	{
		moveLight1 = !moveLight1;
	}
	if (Input()->GetPressKey('U'))
	{
		rotateLight2 = !rotateLight2;
	}
	if (Input()->GetPressKey('E'))
	{
		moveLight1WithCam = !moveLight1WithCam;
	}

	if (moveLight1)
	{
		light1Timer += FDeltaTime();

		auto& light = m_renderer->LightSystem()->GetLight(shadowLight1);

		//light.pos = light.pos + Vec3(0, 0, 30) * sin(2 * counter);
		light.pos.z = 30 * sin(2 * light1Timer);

		m_renderer->LightSystem()->UpdateLight(shadowLight1);
		m_renderer->LightSystem()->UpdateShadow(shadowLight1);

		//lightObj1->Transform().SetPosition(light.pos);

	}

	if (rotateLight2)
	{
		auto& light = m_renderer->LightSystem()->GetLight(shadowLight2);

		auto newDir = Vec4(light.dir, 0) * GetRotationYMatrix(FDeltaTime());

		//light.dir = Vec3(trans.x, trans.y, trans.z);

		//auto shadowVP = m_renderer->LightSystem()->GetShadow(shadowLight2);

		//auto newPos = Vec4(light.pos, 1.0f) * GetRotationYMatrix(FDeltaTime());

		//auto focusPos = Vec3(newPos.x, newPos.y, newPos.z) + light.dir;

		//light.pos = Vec3(newPos.x, newPos.y, newPos.z);
		light.dir = Vec3(newDir.x, newDir.y, newDir.z).Normalize();

		m_renderer->LightSystem()->UpdateLight(shadowLight2);
		m_renderer->LightSystem()->UpdateShadow(shadowLight2);
	}

	if (moveLight1WithCam)
	{
		auto& light = m_renderer->LightSystem()->GetLight(shadowLight1);

		auto cam = m_renderer->GetTargetCamera();

		light.pos = m_renderer->GetTargetCamera()->GetPosition()
			- cam->ViewMatrix().GetUpwardDir().Normalize() * 5
			- cam->ViewMatrix().GetLeftwardDir().Normalize() * 5;

		m_renderer->LightSystem()->UpdateLight(shadowLight1);
		m_renderer->LightSystem()->UpdateShadow(shadowLight1);
	}

	//gerstnerWater->Update(this);

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

	//animModelObj->Update(this);

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
	{
		m_renderer->Present();
	}
		
#else
	m_renderer->Present();
#endif // _DEBUG

	//==================================ImGui=================================================
	// just for test
	// will use wxwidgets or java or C# GUI with remote renderer
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuiShowLight(m_renderer, shadowLight2);
	ImGuiShowLightSystemInfo(m_renderer);

	DX11Global::renderer->m_d3dDeviceContext->OMSetRenderTargets(1, &DX11Global::renderer->m_mainRtv, 0);
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	//==================================ImGui=================================================

	DX11Global::renderer->m_dxgiSwapChain->Present(1, 0);
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

	//lightObj1->Render(m_renderer);

	heightMap->Render(m_renderer);

	wall1->Render(m_renderer);
	wall2->Render(m_renderer);
	wall3->Render(m_renderer);
	wall4->Render(m_renderer);
	wall5->Render(m_renderer);

	spaceCoord->Render(m_renderer);

	if (m_renderer->LightSystem()->BeginShadow(shadowLight1))
	{
		for (auto& obj : renderList)
		{
			obj->Render(m_renderer);
		}
		m_renderer->LightSystem()->EndShadow(shadowLight1);
	}

	if (m_renderer->LightSystem()->BeginShadow(shadowLight2))
	{
		for (auto& obj : renderList)
		{
			obj->Render(m_renderer);
		}
		m_renderer->LightSystem()->EndShadow(shadowLight2);
	}

}