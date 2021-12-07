#include "DX11PostProcessor.h"

#include <Resource.h>
#include <Shader.h>
#include <Buffer.h>

#include <IObject.h>

#include <iostream>

#include <Component/Quad.h>

DX11PostProcessor::DX11PostProcessor(
	DX11Renderer* renderer,
	ID3D11RenderTargetView* mainRtv,
	ID3D11RenderTargetView** freeRtv, 
	ID3D11ShaderResourceView** freeRtvSrv, size_t count,
	ID3D11ShaderResourceView* targetBuffer,
	ID3D11ShaderResourceView* depthBuffer, 
	ID3D11ShaderResourceView* positionBuffer)
{
	m_targetSrv = targetBuffer;
	m_depthSrv = depthBuffer;
	m_positionSrv = positionBuffer;
	m_mainRtv = mainRtv;

	m_d3dContext = renderer->m_d3dDeviceContext;
	m_d3dDevice = renderer->m_d3dDevice;

	m_renderer = renderer;

	auto loop = count >= _DX11PostProcessor_NEED_BUFFER ? _DX11PostProcessor_NEED_BUFFER : count;
	for (size_t i = 0; i < loop; i++)
	{
		m_rtv[i] = freeRtv[i];
		m_rtvSrv[i] = freeRtvSrv[i];
	}

	InitPostProcessingFunction();

	//generate more temp buffer 

	m_quad = NewScreenRectangle();
	m_vs = Resource::Get<VertexShader>(L"VSQuad");
	//m_vs->SetInputLayout ...

	m_lastPS = Resource::Get<PixelShader>(L"PSPresent");

	//allocate 4KB constant buffer for CPU dynamic writing
	//info will pass to this buffer
	std::vector<uint8_t> temp;
	temp.resize(4096);
	m_cbuffer = new DynamicShaderVar(&temp[0], temp.size());

	m_width = renderer->m_bbDesc.Width;
	m_height = renderer->m_bbDesc.Height;
}

DX11PostProcessor::~DX11PostProcessor()
{
	delete m_quad;
	Resource::Release(&m_vs);
	Resource::Release(&m_lastPS);

	for (auto& ps : m_pixelShaders)
	{
		Resource::Release(&ps);
	}

	delete m_cbuffer;
}

void DX11PostProcessor::InitPostProcessingFunction()
{
	//void(__thiscall DX11PostProcessor::*pFunc)() = &DX11PostProcessor::LightBlur;
	m_postProcessingFunc[POST_PROCESSING::LIGHT_BLUR] = &DX11PostProcessor::LightBlur;
	m_pixelShaders.push_back(
		Resource::Get<PixelShader>(L"PostProcessing/Blur/PSBlur")
	);
	m_pixelShaders.push_back(
		Resource::Get<PixelShader>(L"PostProcessing/Thresholding/PSLuma")
	);
	m_pixelShaders.push_back(
		Resource::Get<PixelShader>(L"PostProcessing/Blur/PSCombine")
	);


	m_postProcessingFunc[POST_PROCESSING::LIGHT_SCATTERING] = &DX11PostProcessor::LightScatting;
	m_pixelShaders.push_back(
		Resource::Get<PixelShader>(L"PostProcessing/LightEffect/LightScattering/PSLightScattering")
	);
	m_pixelShaders.push_back(
		Resource::Get<PixelShader>(L"PostProcessing/LightEffect/LightScattering/PSOccludingFromDepth")
	);
	m_pixelShaders.push_back(
		Resource::Get<PixelShader>(L"PostProcessing/LightEffect/LightScattering/PSCombine")
	);
}

void DX11PostProcessor::LightBlur()
{
	struct BlurInfo
	{
		//horizontal or vertical
		union
		{
			uint32_t type;
			float thres;
		};	

		float width;
		float height;
		float padding;
	};

	BlurInfo info = {};

	auto* blurPS = m_pixelShaders[0]->GetNativeHandle();
	auto* thresholdPS = m_pixelShaders[1]->GetNativeHandle();
	auto* combinePS = m_pixelShaders[2]->GetNativeHandle();

	auto* tempRtv1 = m_rtv[0];
	auto* tempRtv2 = m_rtv[1];

	auto* tempRtvSrv1 = m_rtvSrv[0];
	auto* tempRtvSrv2 = m_rtvSrv[1];

	//auto* lastScene = tempRtvSrv1;


	//threshold, target -> rtv1
	info.thres = 0.8f;
	Vec3* lumafactor = (Vec3*)&info.width; 
	*lumafactor = Vec3(0.2126, 0.7152, 0.0722);
	m_cbuffer->Update(&info, sizeof(BlurInfo));
	m_d3dContext->OMSetRenderTargets(1, &tempRtv1, 0);
	m_d3dContext->PSSetShaderResources(PS_SHADER_AVAILABLE_RESOURCE_START_SLOT, 1, &m_targetSrv);
	m_d3dContext->PSSetShader(thresholdPS, 0, 0);
	m_d3dContext->Draw(m_quad->Count(), 0);


	//blur
	info.width = m_width;
	info.height = m_height;
	//horizontal, rtv1 -> rtv2
	info.type = 0;
	m_cbuffer->Update(&info, sizeof(BlurInfo));
	m_d3dContext->OMSetRenderTargets(1, &tempRtv2, 0);
	m_d3dContext->PSSetShaderResources(PS_SHADER_AVAILABLE_RESOURCE_START_SLOT, 1, &tempRtvSrv1);
	m_d3dContext->PSSetShader(blurPS, 0, 0);
	m_d3dContext->Draw(m_quad->Count(), 0);

	//vertical, rtv2 -> rtv1
	info.type = 1;
	m_cbuffer->Update(&info, sizeof(BlurInfo));
	m_d3dContext->OMSetRenderTargets(1, &tempRtv1, 0);
	m_d3dContext->PSSetShaderResources(PS_SHADER_AVAILABLE_RESOURCE_START_SLOT, 1, &tempRtvSrv2);
	//m_d3dContext->PSSetShader(m_lastPS->GetNativeHandle(), 0, 0);
	m_d3dContext->Draw(m_quad->Count(), 0);

	//combine, rtv1 -> rtv2
	//m_cbuffer->Update(&info, sizeof(BlurInfo));
	m_d3dContext->OMSetRenderTargets(1, &tempRtv2, 0);
	m_d3dContext->PSSetShaderResources(PS_SHADER_AVAILABLE_RESOURCE_START_SLOT, 1, &m_targetSrv);
	m_d3dContext->PSSetShaderResources(PS_SHADER_AVAILABLE_RESOURCE_START_SLOT + 1, 1, &tempRtvSrv1);
	m_d3dContext->PSSetShader(combinePS, 0, 0);
	m_d3dContext->Draw(m_quad->Count(), 0);

	m_lastSceneSrv = tempRtvSrv2;
}

#define MAX_SCATTING_LIGHT 5
void DX11PostProcessor::LightScatting()
{
	auto* scPS = m_pixelShaders[3]->GetNativeHandle();
	auto* occPS = m_pixelShaders[4]->GetNativeHandle();
	auto* combinePS = m_pixelShaders[5]->GetNativeHandle();

	auto* target = m_lastSceneSrv;

	auto* tempRtv1 = m_rtv[0];
	auto* tempRtv2 = m_rtv[1];

	auto* tempRtvSrv1 = m_rtvSrv[0];
	auto* tempRtvSrv2 = m_rtvSrv[1];

	
	struct Info
	{
		uint32_t  totalLights;
		Vec3 padding;
		Vec4 lightsPosOnScreen[MAX_SCATTING_LIGHT];
	};

	//auto p = [](const Vec3 pos)

	static Info info = {};

	auto& light = m_renderer->LightSystem()->GetLight(0);

	auto& mvp = m_renderer->GetTargetCamera()->MVP();

	info.totalLights = 2;
	auto p = Vec4(light.pos, 1.0f) * mvp;

	auto w = p.w != 0 ? p.w : 0.0001f;
	//assert(abs(p.w - 1) < 0.001f);
	info.lightsPosOnScreen[0] = { ((p.x / w) + 1) / 2.f, (1 - (p.y / w)) / 2.f, p.z / w, 0 };

	//do occluding
	m_cbuffer->Update(&info, sizeof(Info));
	m_d3dContext->OMSetRenderTargets(1, &tempRtv1, 0);
	m_d3dContext->PSSetShaderResources(PS_SHADER_AVAILABLE_RESOURCE_START_SLOT, 1, &target);
	m_d3dContext->PSSetShaderResources(PS_SHADER_AVAILABLE_RESOURCE_START_SLOT - 1, 1, &m_depthSrv);

	/*D3D11_VIEWPORT vp = {};
	vp.Width = m_width / 2.0f;
	vp.Height = m_height / 2.0f;
	m_d3dContext->RSSetViewports(1, &vp);*/

	m_d3dContext->PSSetShader(occPS, 0, 0);
	m_d3dContext->Draw(m_quad->Count(), 0);

	//do light scatting
	m_d3dContext->OMSetRenderTargets(1, &tempRtv2, 0);
	m_d3dContext->PSSetShaderResources(PS_SHADER_AVAILABLE_RESOURCE_START_SLOT, 1, &tempRtvSrv1);
	m_d3dContext->PSSetShader(scPS, 0, 0);
	m_d3dContext->Draw(m_quad->Count(), 0);



	/*vp.Width = m_width;
	vp.Height = m_height;
	m_d3dContext->RSSetViewports(1, &vp);*/

	//combine
	m_d3dContext->OMSetRenderTargets(1, &tempRtv1, 0);
	m_d3dContext->PSSetShaderResources(PS_SHADER_AVAILABLE_RESOURCE_START_SLOT, 1, &target);
	m_d3dContext->PSSetShaderResources(PS_SHADER_AVAILABLE_RESOURCE_START_SLOT + 1, 1, &tempRtvSrv2);
	m_d3dContext->PSSetShader(combinePS, 0, 0);
	m_d3dContext->Draw(m_quad->Count(), 0);

	m_lastSceneSrv = tempRtvSrv1;
}

void DX11PostProcessor::Apply()
{
	m_d3dContext->IASetInputLayout(m_vs->GetNativeLayoutHandle());
	m_d3dContext->VSSetShader(m_vs->GetNativeHandle(), 0, 0);
	m_d3dContext->IASetVertexBuffers(0, 1, &m_quad->GetNativeHandle(), &m_quad->Stride(), &m_quad->Offset());

	m_d3dContext->PSSetConstantBuffers(PS_SHADER_AVAILABLE_CBUFFER_START_SLOT, 1, &m_cbuffer->GetNativeHandle());

	m_lastSceneSrv = m_targetSrv;
	
	for (auto& m : m_list)
	{
		((*this).*(m_postProcessingFunc[m]))();
	}

	//===================last present===============================================================
	m_d3dContext->OMSetRenderTargets(1, &m_mainRtv, 0);
	m_d3dContext->PSSetShaderResources(PS_SHADER_AVAILABLE_RESOURCE_START_SLOT, 1, &m_lastSceneSrv);
	m_d3dContext->PSSetShader(m_lastPS->GetNativeHandle(), 0, 0);
	m_d3dContext->Draw(m_quad->Count(), 0);
}
