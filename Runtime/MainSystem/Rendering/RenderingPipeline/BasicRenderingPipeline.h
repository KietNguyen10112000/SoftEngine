#pragma once

#include "RenderingPipeline.h"

NAMESPACE_BEGIN

class BasicRenderingPass : public RenderingPass
{
public:
	struct ObjectData
	{
		Mat4 transform;
	};

	SharedPtr<GraphicsDepthStencilBuffer>	m_depthBuffer;

	SharedPtr<GraphicsPipeline>				m_pipeline;
	SharedPtr<GraphicsConstantBuffer>		m_objectBuffer;
	SharedPtr<GraphicsConstantBuffer>		m_cameraBuffer;

	ObjectData								m_objectData;

	BasicRenderingPass();

	virtual void Initialize(RenderingPipeline* pipeline, RenderingPass* prevPass, RenderingPass* nextPass) override;

	virtual void Run(RenderingPipeline* pipeline) override;
};

class BasicRenderingPipeline : public RenderingPipeline
{
public:
	RenderingComponent** m_input = nullptr;
	size_t m_inputCount = 0;

	std::vector<RenderingComponent*> m_basicModel3Ds;

	BasicRenderingPipeline();

	// Inherited via RenderingPipeline
	virtual void SetInput(RenderingComponent** components, size_t count) override;
};

NAMESPACE_END