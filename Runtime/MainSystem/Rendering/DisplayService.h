#pragma once

#include "Modules/Graphics/Graphics.h"

NAMESPACE_BEGIN

class DisplayService
{
public:
	DisplayService();

	void Begin();
	void End();
	void Display(const SharedPtr<GraphicsShaderResource>& resource, GRAPHICS_VIEWPORT viewport);

};

NAMESPACE_END