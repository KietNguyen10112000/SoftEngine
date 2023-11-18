#pragma once

#include "Core/TypeDef.h"

#include "Core/Memory/Memory.h"
#include "Core/Structures/STD/STDContainers.h"

#include "Modules/Graphics/Graphics.h"

NAMESPACE_BEGIN

class RenderingPipeline;
class RenderingComponent;

class RenderingPass
{
public:
	inline virtual ~RenderingPass() {};

	virtual void Initialize(RenderingPipeline* pipeline, RenderingPass* prevPass, RenderingPass* nextPass) = 0;
	virtual void Run(RenderingPipeline* pipeline) = 0;

};

class RenderingPipeline
{
private:
	GraphicsRenderTarget* m_output = nullptr;
	GraphicsDepthStencilBuffer* m_outputDepthBuffer = nullptr;
	RenderingSystem* m_renderingSystem = nullptr;
	std::vector<RenderingPass*> m_passes;

public:
	inline virtual ~RenderingPipeline()
	{
		for (auto& pass : m_passes)
		{
			delete pass;
		}
	}

	virtual void SetInput(RenderingComponent** components, size_t count) = 0;

	inline void AddRenderingPass(RenderingPass* pass)
	{
		assert(pass != nullptr);
		m_passes.push_back(pass);
	}

	inline void AddTopRenderingPass(RenderingPass* pass)
	{
		assert(pass != nullptr);
		m_passes.insert(m_passes.begin(), pass);
	}

	inline void Bake(GraphicsRenderTarget* target, GraphicsDepthStencilBuffer* depthBuffer, RenderingSystem* system)
	{
		if (m_output == target && m_renderingSystem == system) return;
		m_output = target;
		m_outputDepthBuffer = depthBuffer;
		m_renderingSystem = system;

		for (auto& pass : m_passes)
		{
			RenderingPass* prevPass = nullptr;
			RenderingPass* nextPass = nullptr;
			if (pass != m_passes.back())
			{
				nextPass = *(&pass + 1);
			}

			if (pass != m_passes.front())
			{
				prevPass = *(&pass - 1);
			}

			pass->Initialize(this, prevPass, nextPass);
		}
	}

	inline void Run()
	{
		for (auto& pass : m_passes)
		{
			pass->Run(this);
		}
	}

	inline auto GetOutput()
	{
		return m_output;
	}

	inline auto GetOutputDepthBuffer()
	{
		return m_outputDepthBuffer;
	}

	inline auto GetRenderingSystem()
	{
		return m_renderingSystem;
	}

};

NAMESPACE_END