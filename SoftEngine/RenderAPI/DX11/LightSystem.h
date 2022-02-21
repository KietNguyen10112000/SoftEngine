#pragma once

#define SHADOW_MAP_SIZE 4096

#include <ILightSystem.h>

#include "ShaderVar.h"
#include "DX11Global.h"
#include "Renderer.h"

#include <iostream>

#include <Components/Quad.h>

#include <IObject.h>

class DeferredRenderer;

class DX11LightSystem : public LightSystem
{
protected:
	friend class DeferredRenderer;

public:
	ID3D11Buffer* m_lightsBuffer = nullptr;
	ID3D11ShaderResourceView* m_lightsBufferSrv = nullptr;

	ID3D11Buffer* m_shadowLightsBuffer = nullptr;
	ID3D11ShaderResourceView* m_shadowLightsBufferSrv = nullptr;

	ID3D11DepthStencilView* m_shadowDepthMap = nullptr;
	ID3D11ShaderResourceView* m_shadowDepthMapSrv = nullptr;
	//to disable depth test
	ID3D11DepthStencilState* m_dsvState = nullptr;

	ID3D11SamplerState* m_shadowMapSampler = nullptr;
	ID3D11RasterizerState* m_rasterizerState = nullptr;


	//VertexShader* m_vs = nullptr;
	//VertexShader* m_pointLightShadowVS = nullptr;
	GeometryShader* m_pointLightShadowGS = nullptr;

	D3D11_VIEWPORT m_vp[6] = {};

	ShaderVar* m_lightSysInfo = nullptr;

	ShaderVar* m_gsShadowInfo = nullptr;

	ShaderVar* m_lightVP = nullptr;

	VertexBuffer* m_quad = nullptr;
	VertexShader* m_vsClearDepth = nullptr;
	PixelShader* m_psClearDepth = nullptr;
	GeometryShader* m_gsClearDepth = nullptr;

	GeometryShader* m_gsCSMClearDepth = nullptr;
	GeometryShader* m_gsCSMLightShadow = nullptr;

	struct ShadowInfo
	{
		uint32_t index = UINT32_MAX;
		Vec3 padding;
	};

	ShadowInfo m_currentPointLightShadow;

	RenderPipeline1 m_rpl = {};
	bool m_doneBeginShadow = 0;

public:
	inline DX11LightSystem();
	inline ~DX11LightSystem();

private:
	void InitLightsBuffer();
	void InitShadowLightsBuffer();
	void InitShadowMap();

protected:
	inline void BeginCSMShadow(ID3D11DeviceContext* context, Light& light, LightSystem::LightShadow& shadow);

public:
	inline virtual bool BeginShadow(LightID id) override;
	inline virtual void EndShadow(LightID id) override;

public:
	inline virtual void Update() override;

};

inline DX11LightSystem::DX11LightSystem() : LightSystem(SHADOW_MAP_SIZE)
{
	InitLightsBuffer();
	InitShadowLightsBuffer();
	InitShadowMap();

	//LightSystemInfo temp;
	m_info.depthBias = 0;
	m_info.offsetPixelLightFactor.x = 2.486f;
	m_lightSysInfo = new ShaderVar(&m_info, sizeof(LightSystemInfo));
	m_lightSysInfo->Update(&m_info, sizeof(LightSystemInfo));

	Mat4x4 temp2;
	m_lightVP = new ShaderVar(&temp2, sizeof(Mat4x4));

	m_quad = NewScreenRectangle();
	
	m_vsClearDepth = Resource::Get<VertexShader>(L"Shadow/v2/VSClearDepth");
	m_gsClearDepth = Resource::Get<GeometryShader>(L"Shadow/v2/GSClearDepth");
	m_psClearDepth = Resource::Get<PixelShader>(L"Shadow/v2/PSClearDepth");

	m_vsClearDepth->SetInputLayout(
		R"(
		struct VS_INPUT
		{
			Vec3 pos; POSITION, PER_VERTEX #
			Vec2 textCoord; TEXTCOORD, PER_VERTEX #
		};
		)"
	);

	for (size_t i = 0; i < 6; i++)
	{
		m_vp[i] = DX11Global::renderer->m_viewport;
	}

	DX11Global::renderer->m_d3dDeviceContext->PSSetSamplers(1, 1, &m_shadowMapSampler);

	DX11Global::renderer->m_d3dDeviceContext->ClearDepthStencilView(m_shadowDepthMap, D3D11_CLEAR_DEPTH, 1, 0);

	//cascade shadow mapping clear depth
	m_gsCSMClearDepth = Resource::Get<GeometryShader>(L"Shadow/v2/GSCSMClearDepth");
	m_gsCSMLightShadow = Resource::Get<GeometryShader>(L"Shadow/v2/GSCSMShadow");
}

inline DX11LightSystem::~DX11LightSystem()
{
	m_lightsBuffer->Release();
	m_lightsBufferSrv->Release();

	m_shadowLightsBuffer->Release();
	m_shadowLightsBufferSrv->Release();

	//Resource::Release(&m_vs);
	Resource::Release(&m_pointLightShadowGS);
	//Resource::Release(&m_pointLightShadowVS);

	m_shadowDepthMap->Release();
	m_shadowDepthMapSrv->Release();
	if(m_dsvState) m_dsvState->Release();

	m_shadowMapSampler->Release();
	m_rasterizerState->Release();

	delete m_gsShadowInfo;
	delete m_lightSysInfo;
	delete m_lightVP;

	delete m_quad;
	Resource::Release(&m_vsClearDepth);
	Resource::Release(&m_psClearDepth);
	Resource::Release(&m_gsClearDepth);

	Resource::Release(&m_gsCSMClearDepth);
	Resource::Release(&m_gsCSMLightShadow);
}

inline void DX11LightSystem::InitLightsBuffer()
{
	D3D11_BUFFER_DESC sbDesc = {};
	sbDesc.ByteWidth = sizeof(Light) * MAX_LIGHT;
	sbDesc.Usage = D3D11_USAGE_DYNAMIC;
	sbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	sbDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sbDesc.StructureByteStride = sizeof(Light);

	auto hr = DX11Global::renderer->m_d3dDevice->CreateBuffer(&sbDesc, 0, &m_lightsBuffer);

	if (FAILED(hr))
	{
		std::wcout << L"CreateBuffer() failed.\n";
		exit(-1);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = MAX_LIGHT;

	hr = DX11Global::renderer->m_d3dDevice->CreateShaderResourceView(m_lightsBuffer, &srvDesc, &m_lightsBufferSrv);

	if (FAILED(hr))
	{
		std::wcout << L"CreateShaderResourceView() failed.\n";
		exit(-1);
	}

	DX11Global::renderer->m_d3dDeviceContext->GSSetShaderResources(0, 1, &m_lightsBufferSrv);
	//DX11Global::renderer->m_d3dDeviceContext->PSSetShaderResources(0, 1, &m_lightsBufferSrv);
}

inline void DX11LightSystem::InitShadowLightsBuffer()
{
	D3D11_BUFFER_DESC sbDesc = {};
	sbDesc.ByteWidth = sizeof(LightShadow) * MAX_SHADOW_LIGHT;
	sbDesc.Usage = D3D11_USAGE_DYNAMIC;
	sbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	sbDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sbDesc.StructureByteStride = sizeof(LightShadow);

	auto hr = DX11Global::renderer->m_d3dDevice->CreateBuffer(&sbDesc, 0, &m_shadowLightsBuffer);

	if (FAILED(hr))
	{
		std::wcout << L"CreateBuffer() failed.\n";
		exit(-1);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = MAX_SHADOW_LIGHT;

	hr = DX11Global::renderer->m_d3dDevice->CreateShaderResourceView(m_shadowLightsBuffer, &srvDesc, &m_shadowLightsBufferSrv);

	if (FAILED(hr))
	{
		std::wcout << L"CreateShaderResourceView() failed.\n";
		exit(-1);
	}

	DX11Global::renderer->m_d3dDeviceContext->GSSetShaderResources(1, 1, &m_shadowLightsBufferSrv);
	//DX11Global::renderer->m_d3dDeviceContext->PSSetShaderResources(1, 1, &m_shadowLightsBufferSrv);
}

inline void DX11LightSystem::InitShadowMap()
{
	/*m_vs = Resource::Get<VertexShader>(L"Shadow/VSShadow");
	m_vs->SetInputLayout(R"(
		struct VS_INPUT
		{
			Vec3 position; POSITION, PER_VERTEX #
			Vec2 textcoord; TEXTCOORD, PER_VERTEX #
		};
	)");*/

	HRESULT hr = S_OK;

	DX11Renderer* renderer = DX11Global::renderer;
	ID3D11Device* device = DX11Global::renderer->m_d3dDevice;

	ID3D11Texture2D* depthBuffer = NULL;

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_TEXTURE2D_DESC));

	uint32_t dim = m_shadowMapSize * MIN_SHADOW_BLOCK_SIZE;

	depthStencilDesc.Width = dim;
	depthStencilDesc.Height = dim;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	hr = device->CreateTexture2D(&depthStencilDesc, NULL, &depthBuffer);

	if (FAILED(hr))
	{
		if (depthBuffer) depthBuffer->Release();
		exit(-1);
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	descDSV.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	descDSV.Flags = 0;

	hr = device->CreateDepthStencilView(depthBuffer, &descDSV, &m_shadowDepthMap);
	
	if (FAILED(hr))
	{
		if (depthBuffer) depthBuffer->Release();
		exit(-1);
	}

	DX11Global::renderer->m_d3dDeviceContext->ClearDepthStencilView(m_shadowDepthMap, D3D11_CLEAR_DEPTH, 1.0f, 0);

	D3D11_DEPTH_STENCIL_DESC dsDesc;

	// Depth test parameters
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil test parameters
	dsDesc.StencilEnable = FALSE;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	device->CreateDepthStencilState(&dsDesc, &m_dsvState);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	hr = device->CreateShaderResourceView(depthBuffer, &srvDesc, &m_shadowDepthMapSrv);
	if (FAILED(hr))
	{
		if (depthBuffer) depthBuffer->Release();
		exit(-1);
	}

	if (depthBuffer) depthBuffer->Release();


	//config sampler
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	device->CreateSamplerState(&samplerDesc, &m_shadowMapSampler);
	if (FAILED(hr))
	{
		exit(-1);
	}

	D3D11_RASTERIZER_DESC RasterizerDesc;
	ZeroMemory(&RasterizerDesc, sizeof(RasterizerDesc));
	RasterizerDesc.FillMode = D3D11_FILL_SOLID;
	RasterizerDesc.CullMode = D3D11_CULL_BACK;//D3D11_CULL_FRONT;
	RasterizerDesc.FrontCounterClockwise = FALSE;
	RasterizerDesc.DepthBias = 0;
	RasterizerDesc.SlopeScaledDepthBias = 10.f;// 5.0f;//3.5f;//2;
	RasterizerDesc.DepthBiasClamp = 0.0003f;//0.0005f;
	RasterizerDesc.DepthClipEnable = TRUE;
	RasterizerDesc.ScissorEnable = FALSE;
	RasterizerDesc.MultisampleEnable = FALSE;
	RasterizerDesc.AntialiasedLineEnable = FALSE;

	device->CreateRasterizerState(&RasterizerDesc, &m_rasterizerState);


	//=================================================================================================================
	//for point light

	m_pointLightShadowGS = Resource::Get<GeometryShader>(L"Shadow/v2/GSPointLight");
	/*m_pointLightShadowVS = Resource::Get<VertexShader>(L"Shadow/VSPointLightShadow");

	m_pointLightShadowVS->SetInputLayout(
		R"(
		struct VS_INPUT
		{
			Vec3 position; POSITION, PER_VERTEX #
		};
		)"
	);*/

	m_gsShadowInfo = new ShaderVar(&m_currentPointLightShadow, sizeof(ShadowInfo));

	DX11Global::renderer->m_d3dDeviceContext->GSSetConstantBuffers(0, 1, &m_gsShadowInfo->GetNativeHandle());

}

inline void DX11LightSystem::Update()
{
	if (m_needUpdateInfo)
	{
		m_info.numLight = m_activeLights.size();
		m_lightSysInfo->Update(&m_info, sizeof(LightSystemInfo));
		m_needUpdateInfo = false;
	}

	if (m_updateActiveLights.size() != 0)
	{
		//LightSystemInfo info;
		//m_lightSysInfo.environmentAmbient = { 0.2f, 0.2f, 0.2f };
		//if (m_needUpdateInfo)
		//{
		m_info.numLight = m_activeLights.size();
		m_lightSysInfo->Update(&m_info, sizeof(LightSystemInfo));
		m_needUpdateInfo = false;
		//}

		D3D11_MAPPED_SUBRESOURCE mappedResource;

		DX11Global::renderer->m_d3dDeviceContext->Map(m_lightsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		::memcpy(mappedResource.pData, &m_activeLights[0], m_activeLights.size() * sizeof(Light));
		DX11Global::renderer->m_d3dDeviceContext->Unmap(m_lightsBuffer, 0);

		m_updateActiveLights.clear();
	}

	if (m_updateActiveShadowLights.size() != 0)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		DX11Global::renderer->m_d3dDeviceContext->Map(m_shadowLightsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		::memcpy(mappedResource.pData, &m_activeLightShadow[0], m_activeLightShadow.size() * sizeof(LightShadow));
		DX11Global::renderer->m_d3dDeviceContext->Unmap(m_shadowLightsBuffer, 0);

		m_updateActiveShadowLights.clear();
	}
}

inline void DX11LightSystem::BeginCSMShadow(ID3D11DeviceContext* context, Light& light, LightSystem::LightShadow& shadow)
{
	context->OMSetRenderTargets(0, nullptr, m_shadowDepthMap);
	context->OMSetDepthStencilState(m_dsvState, 0);

	for (size_t i = 0; i < ShadowMap_NUM_CASCADE; i++)
	{
		auto& vp = m_vp[i];

		vp.TopLeftX = shadow.uvOffset[i].x * SHADOW_MAP_SIZE;
		vp.TopLeftY = shadow.uvOffset[i].y * SHADOW_MAP_SIZE;

		vp.Width = 1 / shadow.texelDim.x;
		vp.Height = 1 / shadow.texelDim.y;
	}

	context->VSSetShader(m_vsClearDepth->GetNativeHandle(), 0, 0);
	context->GSSetShader(m_gsCSMClearDepth->GetNativeHandle(), 0, 0);
	context->PSSetShader(m_psClearDepth->GetNativeHandle(), 0, 0);
	context->RSSetViewports(ShadowMap_NUM_CASCADE, m_vp);

	context->IASetInputLayout(m_vsClearDepth->GetNativeLayoutHandle());
	context->IASetVertexBuffers(0, 1, &m_quad->GetNativeHandle(), &m_quad->Stride(), &m_quad->Offset());
	context->Draw(m_quad->Count(), 0);


	context->OMSetDepthStencilState(0, 0);
	//context->VSSetShader(m_pointLightShadowVS->GetNativeHandle(), 0, 0);
	context->GSSetShader(m_gsCSMLightShadow->GetNativeHandle(), 0, 0);
	context->PSSetShader(0, 0, 0);

	context->RSSetState(m_rasterizerState);

	m_currentPointLightShadow.index = light.activeShadowIndex;
	m_gsShadowInfo->Update(&m_currentPointLightShadow, sizeof(ShadowInfo));
}

inline bool DX11LightSystem::BeginShadow(LightID id)
{
	m_doneBeginShadow = false;

	auto& light = GetLight(id);

	if (light.activeShadowIndex == UINT32_MAX) return false;

	auto& shadow = m_activeLightShadow[light.activeShadowIndex];

	auto context = DX11Global::renderer->m_d3dDeviceContext;

	//((DeferredRenderer*)DX11Global::renderer)->m_currentRpl = nullptr;

	if (light.type == LIGHT_TYPE::POINT_LIGHT)
	{
		context->OMSetRenderTargets(0, nullptr, m_shadowDepthMap);
		context->OMSetDepthStencilState(m_dsvState, 0);

		//clear depth buffer at 6 regions
		for (size_t i = 0; i < 6; i++)
		{
			auto& vp = m_vp[i];

			vp.TopLeftX = shadow.uvOffset[i].x * SHADOW_MAP_SIZE;
			vp.TopLeftY = shadow.uvOffset[i].y * SHADOW_MAP_SIZE;

			vp.Width = 1 / shadow.texelDim.x;
			vp.Height = 1 / shadow.texelDim.y;
		}

		context->VSSetShader(m_vsClearDepth->GetNativeHandle(), 0, 0);
		context->GSSetShader(m_gsClearDepth->GetNativeHandle(), 0, 0);
		context->PSSetShader(m_psClearDepth->GetNativeHandle(), 0, 0);
		context->RSSetViewports(6, m_vp);

		context->IASetInputLayout(m_vsClearDepth->GetNativeLayoutHandle());
		context->IASetVertexBuffers(0, 1, &m_quad->GetNativeHandle(), &m_quad->Stride(), &m_quad->Offset());
		context->Draw(m_quad->Count(), 0);
		//end clear

		context->OMSetDepthStencilState(0, 0);
		//context->VSSetShader(m_pointLightShadowVS->GetNativeHandle(), 0, 0);
		context->GSSetShader(m_pointLightShadowGS->GetNativeHandle(), 0, 0);
		context->PSSetShader(0, 0, 0);

		context->RSSetState(m_rasterizerState);

		m_currentPointLightShadow.index = light.activeShadowIndex;
		m_gsShadowInfo->Update(&m_currentPointLightShadow, sizeof(ShadowInfo));

		m_doneBeginShadow = true;
	}
	else if (light.type == LIGHT_TYPE::SPOT_LIGHT || light.type == LIGHT_TYPE::DIRECTIONAL_LIGHT)
	{
		context->OMSetRenderTargets(0, nullptr, m_shadowDepthMap);
		context->OMSetDepthStencilState(m_dsvState, 0);

		//clear depth buffer at 1 region
		auto& vp = m_vp[0];

		vp.TopLeftX = shadow.uvOffset[0].x * SHADOW_MAP_SIZE;
		vp.TopLeftY = shadow.uvOffset[0].y * SHADOW_MAP_SIZE;

		vp.Width = 1 / shadow.texelDim.x;
		vp.Height = 1 / shadow.texelDim.y;

		context->IASetInputLayout(m_vsClearDepth->GetNativeLayoutHandle());
		context->VSSetShader(m_vsClearDepth->GetNativeHandle(), 0, 0);
		context->PSSetShader(m_psClearDepth->GetNativeHandle(), 0, 0);
		context->RSSetViewports(1, m_vp);
		context->IASetVertexBuffers(0, 1, &m_quad->GetNativeHandle(), &m_quad->Stride(), &m_quad->Offset());
		context->Draw(m_quad->Count(), 0);
		//end clear

		context->OMSetDepthStencilState(0, 0);
		//context->IASetInputLayout(m_vs->GetNativeLayoutHandle());
		//context->VSSetShader(m_vs->GetNativeHandle(), 0, 0);
		context->PSSetShader(0, 0, 0);

		context->VSSetConstantBuffers(1, 1, &m_lightVP->GetNativeHandle());

		m_lightVP->Update(&shadow.viewProj[0], sizeof(Mat4x4));

		//context->PSSetSamplers(1, 1, &m_shadowMapSampler);
		context->RSSetState(m_rasterizerState);

		m_doneBeginShadow = true;
	}
	else if (light.type == LIGHT_TYPE::CSM_DIRECTIONAL_LIGHT)
	{
		BeginCSMShadow(context, light, shadow);

		m_doneBeginShadow = true;
	}

	if (m_doneBeginShadow)
	{
		auto func = [](
			IRenderer* renderer, 
			RenderPipeline* ownRpl, 
			RenderPipeline* rpl, 
			VertexBuffer** vb,
			IndexBuffer** ib, 
			uint32_t* count) {
			
				VertexShader* vs = ownRpl->GetVS();

				if (vs != rpl->GetVS())
				{
					//caching
					vs = rpl->GetVS();
					ownRpl->SetVS(vs);
					auto context = DX11Global::renderer->m_d3dDeviceContext;
					context->IASetInputLayout(vs->GetNativeLayoutHandle());
					context->VSSetShader(vs->GetNativeHandle(), 0, 0);
				}
		};

		m_rpl.SetVS(0);
		DX11Global::renderer->RedirectRenderPipeline(&m_rpl, func);
	}

	return m_doneBeginShadow;
}

inline void DX11LightSystem::EndShadow(LightID id)
{
	auto context = DX11Global::renderer->m_d3dDeviceContext;

	context->RSSetViewports(1, &DX11Global::renderer->m_viewport);

	context->OMSetRenderTargets(1, &DX11Global::renderer->m_mainRtv, DX11Global::renderer->m_dsv);

	context->RSSetState(DX11Global::renderer->m_rasterizerState);
	context->PSSetSamplers(1, 1, &m_shadowMapSampler);

	context->GSSetShader(0, 0, 0);

	if (m_doneBeginShadow)
	{
		DX11Global::renderer->RestoreRenderPipeline();
	}

	auto& light = GetLight(id);
	if (light.type == LIGHT_TYPE::SPOT_LIGHT || light.type == LIGHT_TYPE::DIRECTIONAL_LIGHT)
	{
		context->VSSetConstantBuffers(1, 1, &ICamera::shaderMVP->GetNativeHandle());
	}

	m_doneBeginShadow = false;
}
