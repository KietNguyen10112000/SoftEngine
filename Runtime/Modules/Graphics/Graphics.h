#pragma once
#include "Core/TypeDef.h"

#include "Core/Pattern/Singleton.h"
#include "Core/Memory/SmartPointers.h"

#include "GraphicsFundamental.h"

NAMESPACE_BEGIN

enum class GRAPHICS_BACKEND_API
{
	DX12
};

class DebugGraphics;
class RenderingSystem;
class Camera;

class API Graphics : public Singleton<Graphics>
{
public:
	DebugGraphics* m_debugGraphics = nullptr;

	RenderingSystem* m_bindedRdSys = nullptr;

	uint32_t m_windowWidth;
	uint32_t m_windowHeight;

public:
	virtual ~Graphics() {};

public:
	static int Initilize(void* windowNativeHandle, GRAPHICS_BACKEND_API backendAPI);
	static void Finalize();

public:
	virtual SharedPtr<GraphicsPipeline> CreateRasterizerPipeline(const GRAPHICS_PIPELINE_DESC& desc) = 0;

	virtual void CreateShaderResources(
		uint32_t numShaderResources, 
		const GRAPHICS_SHADER_RESOURCE_DESC* descs, 
		SharedPtr<GraphicsShaderResource>* output
	) = 0;

	virtual void CreateConstantBuffers(
		uint32_t numConstantBuffers,
		const GRAPHICS_CONSTANT_BUFFER_DESC* descs,
		SharedPtr<GraphicsConstantBuffer>* output
	) = 0;

	virtual void CreateRenderTargets(
		uint32_t numRenderTargets, 
		const GRAPHICS_RENDER_TARGET_DESC* desc, 
		SharedPtr<GraphicsRenderTarget> * output
	) = 0;

	virtual SharedPtr<GraphicsDepthStencilBuffer> CreateDepthStencilBuffer(const GRAPHICS_DEPTH_STENCIL_BUFFER_DESC& desc) = 0;

	//virtual SharedPtr<GraphicsPipelineInput> CreatePipelineInput(const GRAPHICS_PIPELINE_INPUT_DESC& desc) = 0;
	virtual SharedPtr<GraphicsVertexBuffer> CreateVertexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc) = 0;

	virtual SharedPtr<GraphicsIndexBuffer> CreateIndexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc) = 0;

	virtual void SetGraphicsPipeline(GraphicsPipeline* graphicsPipeline) = 0;

	virtual void SetDrawParams(GraphicsParams* params) = 0;

	virtual void SetRenderTargets(uint32_t numRT, GraphicsRenderTarget** rtv, GraphicsDepthStencilBuffer* dsv) = 0;
	virtual void UnsetRenderTargets(uint32_t numRT, GraphicsRenderTarget** rtv, GraphicsDepthStencilBuffer* dsv) = 0;

	virtual void ClearRenderTarget(GraphicsRenderTarget* rtv, Vec4 clearColor, uint32_t numRect, const AARect* rects) = 0;
	virtual void ClearDepthStencil(GraphicsDepthStencilBuffer* dsv, uint32_t numRect, const AARect* rects) = 0;

	virtual void DrawInstanced(
		uint32_t numVertexBuffers,
		GraphicsVertexBuffer** vertexBuffers, 
		uint32_t vertexCountPerInstance, 
		uint32_t instanceCount,
		uint32_t startVertexLocation,
		uint32_t startInstanceLocation
	) = 0;

	virtual void DrawIndexedInstanced(
		GraphicsIndexBuffer* indexBuffer,
		uint32_t indexCountPerInstance,
		uint32_t numVertexBuffers,
		GraphicsVertexBuffer** vertexBuffers,
		uint32_t vertexCountPerInstance,
		uint32_t instanceCount,
		uint32_t startVertexLocation,
		uint32_t startInstanceLocation
	) = 0;

	virtual GraphicsRenderTarget* GetScreenRenderTarget() = 0;
	virtual GraphicsDepthStencilBuffer* GetScreenDepthStencilBuffer() = 0;

	// the fence in term of modern graphics programming
	// after each draw call, each EndFrame() call, fence value will be increased
	virtual uint64_t GetCurrentFenceValue() = 0;
	virtual void WaitForFenceValue(uint64_t value) = 0;

	virtual void BeginFrame() = 0;
	virtual void EndFrame(bool vsync) = 0;

public:
	inline auto GetDebugGraphics()
	{
		return m_debugGraphics;
	}

	inline void Bind(RenderingSystem* sys)
	{
		m_bindedRdSys = sys;
	}

	inline auto GetRenderingSystem()
	{
		return m_bindedRdSys;
	}

	inline auto GetWindowWidth()
	{
		return m_windowWidth;
	}

	inline auto GetWindowHeight()
	{
		return m_windowHeight;
	}

};

NAMESPACE_END