#pragma once

#include "Modules/Graphics/Graphics.h"

NAMESPACE_BEGIN

class DisplayService : public Singleton<DisplayService>
{
public:
	struct CBuffer
	{
		Vec4 vertices[36];
	};

	SharedPtr<GraphicsPipeline> m_pipeline;

	SharedPtr<GraphicsConstantBuffer> m_constantBuffer;

	CBuffer m_cbuffer;

	DisplayService();

	void Begin();
	void End();
	void Display(SharedPtr<GraphicsShaderResource>& resource, GRAPHICS_VIEWPORT viewport);

};

NAMESPACE_END