#include "BuiltinConstantBuffers.h"

NAMESPACE_BEGIN

BuiltinConstantBuffers::BuiltinConstantBuffers()
{
	auto graphics = Graphics::Get();

	GRAPHICS_CONSTANT_BUFFER_DESC cbDesc0 = {};
	cbDesc0.bufferSize = sizeof(SceneData);
	cbDesc0.perferNumRoom = 1;
	graphics->CreateConstantBuffers(1, &cbDesc0, &m_sceneBuffer);

	GRAPHICS_CONSTANT_BUFFER_DESC cbDesc1 = {};
	cbDesc1.bufferSize = sizeof(CameraData);
	cbDesc1.perferNumRoom = 128;
	graphics->CreateConstantBuffers(1, &cbDesc1, &m_cameraBuffer);
}

NAMESPACE_END