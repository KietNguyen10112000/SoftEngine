#pragma once

#include "RenderingPipeline.h"

NAMESPACE_BEGIN

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