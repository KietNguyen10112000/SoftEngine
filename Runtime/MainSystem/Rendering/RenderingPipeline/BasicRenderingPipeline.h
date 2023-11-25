#pragma once

#include "RenderingPipeline.h"

NAMESPACE_BEGIN

class BasicSkyRenderingPass : public RenderingPass
{
public:
	SharedPtr<GraphicsPipeline>				m_pipeline;
	SharedPtr<GraphicsConstantBuffer>		m_cameraBuffer;

	BasicSkyRenderingPass();

	virtual void Initialize(RenderingPipeline* pipeline, RenderingPass* prevPass, RenderingPass* nextPass) override;

	virtual void Run(RenderingPipeline* pipeline) override;
};

class AnimMeshRenderer;

class BasicAnimModelRenderingPass : public RenderingPass
{
public:
	constexpr static size_t MAX_BONES = 512;

	SharedPtr<GraphicsPipeline>	m_animModel4Pipeline;
	SharedPtr<GraphicsPipeline>	m_animModel8Pipeline;
	SharedPtr<GraphicsPipeline>	m_animModel16Pipeline;

	SharedPtr<GraphicsConstantBuffer>		m_objectBuffer;
	SharedPtr<GraphicsConstantBuffer>		m_cameraBuffer;

	SharedPtr<GraphicsConstantBuffer>		m_bonesBuffer;

	BasicAnimModelRenderingPass();

	virtual void Initialize(RenderingPipeline* pipeline, RenderingPass* prevPass, RenderingPass* nextPass) override;

	virtual void Run(RenderingPipeline* pipeline) override;

	void Render(std::vector<AnimMeshRenderer*>& input, SharedPtr<GraphicsPipeline>& pipeline);
};

class BasicRenderingPass : public RenderingPass
{
public:
	struct ObjectData
	{
		Mat4 transform;
	};

	GraphicsDepthStencilBuffer*				m_depthBuffer;

	SharedPtr<GraphicsPipeline>				m_pipeline;
	SharedPtr<GraphicsConstantBuffer>		m_objectBuffer;
	SharedPtr<GraphicsConstantBuffer>		m_cameraBuffer;

	BasicRenderingPass();

	virtual void Initialize(RenderingPipeline* pipeline, RenderingPass* prevPass, RenderingPass* nextPass) override;

	virtual void Run(RenderingPipeline* pipeline) override;
};

class BasicRenderingPipeline : public RenderingPipeline
{
public:
	struct ObjectData
	{
		Mat4 transform;
	};

	RenderingComponent** m_input = nullptr;
	size_t m_inputCount = 0;

	SharedPtr<GraphicsConstantBuffer>		m_objectBuffer;
	SharedPtr<GraphicsConstantBuffer>		m_cameraBuffer;

	ObjectData								m_objectData;

	std::vector<RenderingComponent*> m_basicModel3Ds;

	std::vector<AnimMeshRenderer*>	m_animMesh4;
	std::vector<AnimMeshRenderer*>	m_animMesh8;
	std::vector<AnimMeshRenderer*>	m_animMesh16;

	BasicRenderingPipeline();

	// Inherited via RenderingPipeline
	virtual void SetInput(RenderingComponent** components, size_t count) override;
};

NAMESPACE_END