#include "Graphics.h"

#include "Core/Memory/Memory.h"

#include "API/API.h"


NAMESPACE_BEGIN

void Graphics::Initilize(void* windowNativeHandle, GRAPHICS_BACKEND_API backendAPI)
{
	if (s_instance.get() != nullptr)
	{
		assert(0);
	}

	Graphics* ret = nullptr;

	switch (backendAPI)
	{

#ifdef _WIN32
	case soft::GRAPHICS_BACKEND_API::DX12:
		ret = rheap::New<dx12::DX12Graphics>(windowNativeHandle);
		ret->m_debugGraphics = rheap::New<dx12::DX12DebugGraphics>();
		s_instance.reset(ret);
		break;
#endif

	default:
		assert(0);
		break;
	}
}

void Graphics::Finalize()
{
	auto graphics = s_instance.release();

	rheap::Delete(graphics->m_debugGraphics);
	rheap::Delete(graphics);
}

NAMESPACE_END