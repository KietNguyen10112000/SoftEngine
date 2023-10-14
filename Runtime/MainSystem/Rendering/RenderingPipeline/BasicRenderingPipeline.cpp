#include "BasicRenderingPipeline.h"

#include "MainSystem/Rendering/Components/Model3DBasicRenderer.h"
#include "MainSystem/Rendering/Components/RENDER_TYPE.h"

NAMESPACE_BEGIN

class BasicRenderingPass : public RenderingPass
{
public:
	SharedPtr<GraphicsDepthStencilBuffer> m_depthBuffer;



	// Inherited via RenderingPass
	virtual void Initialize(RenderingPipeline* pipeline, RenderingPass* prevPass, RenderingPass* nextPass) override
	{
		auto graphics = Graphics::Get();
		auto output = pipeline->GetOutput();
		
		GRAPHICS_SHADER_RESOURCE_DESC outputDesc = {};
		output->GetShaderResource()->GetDesc(&outputDesc);

		// create depth buffer
		GRAPHICS_DEPTH_STENCIL_BUFFER_DESC depthBufferDesc = {};
		depthBufferDesc.format = GRAPHICS_DATA_FORMAT::FORMAT_R32_FLOAT;
		depthBufferDesc.mipLevels = 1;
		depthBufferDesc.width = outputDesc.texture2D.width;
		depthBufferDesc.height = outputDesc.texture2D.height;
		m_depthBuffer = graphics->CreateDepthStencilBuffer(depthBufferDesc);
	}

	virtual void Run(RenderingPipeline* pipeline) override
	{
		assert(dynamic_cast<BasicRenderingPipeline*>(pipeline));

		auto basicPipeline = (BasicRenderingPipeline*)pipeline;
		auto& input = basicPipeline->m_basicModel3Ds;
		auto output = basicPipeline->GetOutput();

		auto graphics = Graphics::Get();
		graphics->SetRenderTargets(1, &output, m_depthBuffer.get());

		for (auto& comp : input)
		{
			auto model = (Model3DBasicRenderer*)comp;
			
		}
	}

};

BasicRenderingPipeline::BasicRenderingPipeline()
{
	AddRenderingPass(new BasicRenderingPass());
}

void BasicRenderingPipeline::SetInput(RenderingComponent** components, size_t count)
{
	m_input = components;
	m_inputCount = count;

	m_basicModel3Ds.clear();
	for (size_t i = 0; i < count; i++)
	{
		auto comp = components[i];
		if (comp->GetRenderType() == RENDER_TYPE_MODEL3D_BASIC_RENDERER)
		{
			m_basicModel3Ds.push_back(comp);
		}
	}
}

NAMESPACE_END