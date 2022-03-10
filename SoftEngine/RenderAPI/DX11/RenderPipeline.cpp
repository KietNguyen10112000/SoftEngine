#include "RenderPipeline.h"
#include "Renderer.h"

#include "ShaderVar.h"

#include <Resource.h>


RenderPipeline::RenderPipeline()
{
	m_refCount = 1;
}

RenderPipeline::~RenderPipeline()
{
	Resource::Release(&m_vertexShader);
	Resource::Release(&m_pixelShader);

	Resource::Release(&m_hs);
	Resource::Release(&m_ds);
	Resource::Release(&m_gs);
}