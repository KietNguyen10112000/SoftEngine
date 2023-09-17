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

	virtual SharedPtr<GraphicsRenderTarget> CreateRenderTarget(const GRAPHICS_RENDER_TARGET_DESC& desc) = 0;

	//virtual SharedPtr<GraphicsPipelineInput> CreatePipelineInput(const GRAPHICS_PIPELINE_INPUT_DESC& desc) = 0;
	virtual SharedPtr<GraphicsVertexBuffer> CreateVertexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc) = 0;

	virtual SharedPtr<GraphicsIndexBuffer> CreateIndexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc) = 0;

	virtual void SetGraphicsPipeline(GraphicsPipeline* graphicsPipeline) = 0;

	virtual void SetDrawParams(GraphicsParams* params) = 0;

	virtual void SetRenderTarget(uint32_t numRT, GraphicsRenderTarget** rtv, GraphicsDepthStencilBuffer* dsv) = 0;

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

};

NAMESPACE_END