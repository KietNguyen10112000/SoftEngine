#pragma once
#include "Renderer.h"
//#include "Shader.h"
#include "ShaderVar.h"
#include <map>

#include "DX11Global.h"

#include "Shader.h"
#include "DX11Resource.h"

class ShaderVar;
class RenderPipeline;
class VertexShader;
class PixelShader;

class HullShader;
class GeometryShader;
class DomainShader;

//8x4 = 24 bytes
struct _RenderPipelineKey
{
	VertexShader* vs = nullptr;

	//not use yet
	HullShader* hs = nullptr;
	DomainShader* ds = nullptr;

	GeometryShader* gs = nullptr;

	PixelShader* ps = nullptr;
};

typedef ID3D11ShaderResourceView NativeResourceHandle;

class RenderPipeline
{
private:
	friend class RenderPipelineManager;

protected:
	uint32_t m_refCount = 1;
	//own
	VertexShader* m_vertexShader = nullptr;
	PixelShader* m_pixelShader = nullptr;

public:
	RenderPipeline();
	virtual ~RenderPipeline();

public:
	inline VertexShader* GetVS() const { return m_vertexShader; };
	inline PixelShader* GetPS() const { return m_pixelShader; };

	inline virtual GeometryShader* GetGS() const { return 0; };
	inline virtual HullShader* GetHS() const { return 0; };

public:
	//surely what you want to do
	//not recommend use
	inline virtual void SetVS(VertexShader* vs) { m_vertexShader = vs; };
	//not recommend use
	inline virtual void SetPS(PixelShader* ps) { m_pixelShader = ps; };
	//not recommend use
	inline virtual void SetGS(GeometryShader* gs) {};
	//not recommend use
	inline virtual void SetHS(HullShader* hs) {};

public:

	//set constant buffer for DX, uniform for OpenGL
	//location is 
	//register in DX
	//glGetUniformLocation() in OpenGL
	//inline static void VSSetVar(ShaderVar* var, int location);

	template<typename T>
	inline static void VSSetVar(T* var, int location)
	{
		DX11Global::renderer->m_d3dDeviceContext->VSSetConstantBuffers(location, 1, &var->m_buffer);
	}

	inline static void PSSetVar(ShaderVar* var, int location);

	inline static void PSSetResource(Texture2D* texture2d, int location);

	inline static void PSSetResources(NativeResourceHandle** resources, int resourcesCount, int startLocation);

	inline static void PSSetResource(TextureCube* textureCube, int location);

	inline virtual void Use();

public:
	inline virtual _RenderPipelineKey GetKey();
	inline virtual void Config(_RenderPipelineKey* key);

};

class RenderPipeline1 : public RenderPipeline
{
private:
	friend class RenderPipelineManager;

protected:
	GeometryShader* m_gs = 0;

public:
	inline virtual void SetGS(GeometryShader* gs) override { m_gs = gs; };

	inline virtual _RenderPipelineKey GetKey() override;
	inline virtual void Config(_RenderPipelineKey* key) override;

};


class RenderPipeline2 : public RenderPipeline1
{
private:
	friend class RenderPipelineManager;

protected:
	HullShader* m_hs = 0;
	DomainShader* m_ds = 0;

public:
	inline virtual _RenderPipelineKey GetKey() override;
	inline virtual void Config(_RenderPipelineKey* key) override;

};

//inline void RenderPipeline::VSSetVar(ShaderVar* var, int location)
//{
//	DX11Global::renderer->m_d3dDeviceContext->VSSetConstantBuffers(location, 1, &var->m_buffer);
//}

inline void RenderPipeline::PSSetVar(ShaderVar* var, int location)
{
	DX11Global::renderer->m_d3dDeviceContext->PSSetConstantBuffers(location, 1, &var->m_buffer);
}

inline void RenderPipeline::PSSetResource(Texture2D* texture2d, int location)
{
	DX11Global::renderer->m_d3dDeviceContext->PSSetShaderResources(location, 1, &texture2d->GetNativeSRVHandle());
}

inline void RenderPipeline::PSSetResources(NativeResourceHandle** resources, int resourcesCount, int startLocation)
{
	DX11Global::renderer->m_d3dDeviceContext->PSSetShaderResources(startLocation, resourcesCount, resources);
}

inline void RenderPipeline::PSSetResource(TextureCube* textureCube, int location)
{
	DX11Global::renderer->m_d3dDeviceContext->PSSetShaderResources(location, 1, &textureCube->GetNativeSRVHandle());
}

inline void RenderPipeline::Use()
{
	auto context = DX11Global::renderer->m_d3dDeviceContext;
	context->IASetInputLayout(m_vertexShader->GetNativeLayoutHandle());
	context->VSSetShader(m_vertexShader->GetNativeHandle(), 0, 0);
	context->PSSetShader(m_pixelShader->GetNativeHandle(), 0, 0);
}

inline _RenderPipelineKey RenderPipeline::GetKey()
{
	_RenderPipelineKey re = {};

	re.vs = m_vertexShader;
	re.ps = m_pixelShader;

	return re;
}

inline void RenderPipeline::Config(_RenderPipelineKey* key)
{
	m_vertexShader = key->vs;
	m_pixelShader = key->ps;
}

inline _RenderPipelineKey RenderPipeline1::GetKey()
{
	_RenderPipelineKey re = {};

	re.vs = m_vertexShader;
	re.ps = m_pixelShader;
	re.gs = m_gs;

	return re;
}

inline _RenderPipelineKey RenderPipeline2::GetKey()
{
	_RenderPipelineKey re = {};

	re.vs = m_vertexShader;
	re.ps = m_pixelShader;
	re.gs = m_gs;
	re.hs = m_hs;
	re.ds = m_ds;

	return re;
}

inline void RenderPipeline1::Config(_RenderPipelineKey* key)
{
	m_vertexShader = key->vs;
	m_pixelShader = key->ps;
	m_gs = key->gs;
}

inline void RenderPipeline2::Config(_RenderPipelineKey* key)
{
	m_vertexShader = key->vs;
	m_pixelShader = key->ps;
	m_gs = key->gs;
	m_hs = key->hs;
	m_ds = key->ds;
}