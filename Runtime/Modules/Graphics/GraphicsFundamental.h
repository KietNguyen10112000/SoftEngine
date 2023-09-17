#pragma once

#include "Core/TypeDef.h"
#include "Math/Math.h"

NAMESPACE_BEGIN

struct GRAPHICS_VIEWPORT
{
	Vec2 topLeft;

	// width, height
	Vec2 size;
};

struct TEXTURE2D_REGION
{
	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t width = 0;
	uint32_t height = 0;
};

struct GRAPHICS_DATA_FORMAT
{
	enum FORMAT
	{
		FORMAT_R32G32B32_FLOAT,
		FORMAT_R32G32B32A32_FLOAT,
		FORMAT_R8G8B8A8_UNORM
	};
};

struct GRAPHICS_CONSTANT_BUFFER_DESC
{
	size_t bufferSize;
};

struct GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC
{
	uint32_t stride;
	uint32_t count;
	//GRAPHICS_DATA_FORMAT::FORMAT format;
};

struct GRAPHICS_SHADER_RESOURCE_TYPE_TEXTURE2D_DESC
{
	enum TEXTURE_TYPE
	{
		TEXTURE_TYPE_NONE,
		TEXTURE_TYPE_RGBA,
		TEXTURE_TYPE_RGB,
		TEXTURE_TYPE_R
	};

	TEXTURE_TYPE type = TEXTURE_TYPE_NONE;

	uint32_t width;
	uint32_t height;
};

struct GRAPHICS_SHADER_RESOURCE_DESC
{
	enum TYPE
	{
		SHADER_RESOURCE_TYPE_NONE,
		SHADER_RESOURCE_TYPE_BUFFER,
		SHADER_RESOURCE_TYPE_TEXTURE2D
	};

	TYPE type = SHADER_RESOURCE_TYPE_NONE;

	union
	{
		GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC buffer;
		GRAPHICS_SHADER_RESOURCE_TYPE_TEXTURE2D_DESC texture2D;
	};
};

using GRAPHICS_RENDER_TARGET_DESC = GRAPHICS_SHADER_RESOURCE_DESC;

struct GRAPHICS_PARAMS_DESC
{
	constexpr static size_t NUM_CONSTANT_BUFFER = 16;
	constexpr static size_t NUM_SHADER_RESOURCE = 16;

	GRAPHICS_CONSTANT_BUFFER_DESC constantBufferDesc[NUM_CONSTANT_BUFFER] = {};
	GRAPHICS_SHADER_RESOURCE_DESC shaderResourceDesc[NUM_SHADER_RESOURCE] = {};
};

struct GRAPHICS_PIPELINE_INPUT_CLASSIFICATION
{
	enum TYPE
	{
		INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		INPUT_CLASSIFICATION_PER_INSTANCE_DATA
	};
};

// just copied from d3d12 =)))
struct GRAPHICS_PIPELINE_INPUT_ELEMENT_DESC
{
	const char* semanticName;
	uint32_t												semanticIndex;
	GRAPHICS_DATA_FORMAT::FORMAT							format;
	uint32_t												inputSlot;
	uint32_t												alignedByteOffset;
	GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::TYPE	inputSlotClass;
	uint32_t												instanceDataStepRate;
};

struct GRAPHICS_PIPELINE_INPUT_DESC
{
	uint32_t numElements = 0;
	GRAPHICS_PIPELINE_INPUT_ELEMENT_DESC elements[16];
};

//struct GRAPHICS_PIPELINE_DEPTH_STENCIL_DESC
//{
//	bool                       depthEnable;
//	D3D12_DEPTH_WRITE_MASK     depthWriteMask;
//	D3D12_COMPARISON_FUNC      depthFunc;
//	BOOL                       stencilEnable;
//	UINT8                      stencilReadMask;
//	UINT8                      stencilWriteMask;
//	D3D12_DEPTH_STENCILOP_DESC frontFace;
//	D3D12_DEPTH_STENCILOP_DESC backFace;
//};

struct GRAPHICS_PIPELINE_OUTPUT_DESC
{
	uint32_t numRenderTarget;
	GRAPHICS_DATA_FORMAT::FORMAT RTVFormat[8];

	//GRAPHICS_DATA_FORMAT::FORMAT DSVFormat;
};

struct GRAPHICS_PRIMITIVE_TOPOLOGY
{
	enum TYPE 
	{
		PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED = 0,
		PRIMITIVE_TOPOLOGY_TYPE_POINT = 1,
		PRIMITIVE_TOPOLOGY_TYPE_LINE = 2,
		PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE = 3,
		PRIMITIVE_TOPOLOGY_TYPE_PATCH = 4
	};
};

struct GRAPHICS_COMPUTE_PIPELINE_DESC
{

};

struct GRAPHICS_PIPELINE_DESC
{
	const char* vs = nullptr;
	const char* ps = nullptr;
	const char* hs = nullptr;
	const char* ds = nullptr;
	const char* gs = nullptr;

	GRAPHICS_PIPELINE_INPUT_DESC inputDesc;
	GRAPHICS_PIPELINE_OUTPUT_DESC outputDesc;

	GRAPHICS_PARAMS_DESC vsParamsDesc;
	GRAPHICS_PARAMS_DESC psParamsDesc;
	GRAPHICS_PARAMS_DESC hsParamsDesc;
	GRAPHICS_PARAMS_DESC dsParamsDesc;
	GRAPHICS_PARAMS_DESC gsParamsDesc;
};

struct GRAPHICS_RAYTRACING_PIPELINE_DESC
{

};

struct GRAPHICS_SHADER_SPACE
{
	enum SPACE
	{
		SHADER_SPACE_VS,
		SHADER_SPACE_PS,
		SHADER_SPACE_HS,
		SHADER_SPACE_DS,
		SHADER_SPACE_GS
	};
};

class GraphicsShaderResource
{
public:
	// only work if GRAPHICS_SHADER_RESOURCE_DESC::TYPE is SHADER_RESOURCE_TYPE_BUFFER
	virtual void UpdateBufferSynch(void* buffer, size_t bufferSize) = 0;

	// only work if GRAPHICS_SHADER_RESOURCE_DESC::TYPE is SHADER_RESOURCE_TYPE_TEXTURE2D
	virtual void UpdateTexture2DSynch(void* buffer, size_t bufferSize, const TEXTURE2D_REGION& region) = 0;

};

class GraphicsRenderTarget
{
//protected:
//	GraphicsShaderResource* m_shaderResource = nullptr;
//
//public:
//	inline auto GetShaderResource()
//	{
//		return m_shaderResource;
//	}

};

class GraphicsDepthStencilBuffer
{
//protected:
//	GraphicsShaderResource* m_shaderResource = nullptr;
//
//public:
//	inline auto GetShaderResource()
//	{
//		return m_shaderResource;
//	}

};

class GraphicsParams
{
public:
	// set ConstantBuffer with async update content
	virtual void SetConstantBuffer(GRAPHICS_SHADER_SPACE::SPACE shaderSpace, ID slotIndex, 
		void* buffer, size_t bufferSize) = 0;

	// set Shader Resource type Buffer with async update content
	virtual void SetShaderResourceBuffer(GRAPHICS_SHADER_SPACE::SPACE shaderSpace, ID slotIndex, 
		GraphicsShaderResource* shaderResource, void* buffer, size_t bufferSize) = 0;

	// set Shader Resource type Texture2D with async update content
	virtual void SetShaderResourceTexture2D(GRAPHICS_SHADER_SPACE::SPACE shaderSpace, ID slotIndex, 
		GraphicsShaderResource* shaderResource, void* buffer, size_t bufferSize, const TEXTURE2D_REGION& region) = 0;

};

class GraphicsVertexBuffer
{

};

class GraphicsIndexBuffer
{

};

//class GraphicsPipelineInput
//{
//public:
//	virtual void SetVertexBuffers(uint32_t numBuffers, GraphicsVertexBuffer** vertexBuffer) = 0;
//	virtual void SetIndexBuffers(uint32_t numBuffers, GraphicsIndexBuffer** indexBuffer) = 0;
//
//};
//
//class GraphicsPipelineOutput
//{
//public:
//	virtual void SetRenderTarget(uint32_t numRT, GraphicsRenderTarget** rtv) = 0;
//	virtual void SetDepthStencilBuffer(GraphicsDepthStencilBuffer* dsv) = 0;
//
//};
//
//struct GraphicsRenderCall
//{
//	GraphicsParams* params;
//	GraphicsPipelineInput* input;
//	GraphicsPipelineOutput* output;
//};

class GraphicsPipeline
{
public:
	virtual GraphicsParams* PrepareRenderParams() = 0;
	
};

NAMESPACE_END