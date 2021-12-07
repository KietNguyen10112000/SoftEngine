#pragma once

#include <PostProcessor.h>

#include "Renderer.h"

#define _DX11PostProcessor_NEED_BUFFER 8

class DX11PostProcessor : public PostProcessor
{
protected:
	enum PS_INDEX
	{
		PS_SHADER_AVAILABLE_RESOURCE_START_SLOT = 4,
		PS_SHADER_AVAILABLE_CBUFFER_START_SLOT = 2
	};

	//not own
	ID3D11Device* m_d3dDevice = 0;
	ID3D11DeviceContext* m_d3dContext = 0;
	//not own
	ID3D11RenderTargetView* m_mainRtv = 0;
	//not own
	ID3D11RenderTargetView* m_rtv[8] = {};
	ID3D11ShaderResourceView* m_rtvSrv[8] = {};
	//not own
	ID3D11ShaderResourceView* m_targetSrv = 0;
	ID3D11ShaderResourceView* m_depthSrv = 0;
	ID3D11ShaderResourceView* m_positionSrv = 0;

	void(__thiscall DX11PostProcessor::* m_postProcessingFunc[10])() = {};

	ID3D11ShaderResourceView* m_lastSceneSrv = 0;

	DX11Renderer* m_renderer = 0;

	//own
	VertexBuffer* m_quad = 0;

	//just need 1 vs but many ps
	VertexShader* m_vs = 0;
	PixelShader* m_lastPS = 0;
	std::vector<PixelShader*> m_pixelShaders;

	DynamicShaderVar* m_cbuffer = 0;

	float m_width = 0;
	float m_height = 0;

public:
	//all freeRtv has same dimensions
	DX11PostProcessor(
		DX11Renderer* renderer,
		ID3D11RenderTargetView* mainRtv,
		ID3D11RenderTargetView** freeRtv, 
		ID3D11ShaderResourceView** freeRtvSrv, size_t count, 
		ID3D11ShaderResourceView* targetBuffer,
		ID3D11ShaderResourceView* depthBuffer, 
		ID3D11ShaderResourceView* positionBuffer);

	~DX11PostProcessor();

private:
	void InitPostProcessingFunction();

public:
	void LightBlur();
	void LightScatting();

public:
	// Inherited via PostProcessor
	virtual void Apply() override;

};