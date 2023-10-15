#pragma once

#include "Modules/Graphics/Graphics.h"

NAMESPACE_BEGIN

class BuiltinConstantBuffers : public Singleton<BuiltinConstantBuffers>
{
public:
#include "BuiltinConstantBuffers.inl"

public:
	SharedPtr<GraphicsConstantBuffer>		m_sceneBuffer;
	SharedPtr<GraphicsConstantBuffer>		m_cameraBuffer;

public:
	BuiltinConstantBuffers();

	inline auto& GetSceneBuffer()
	{
		return m_sceneBuffer;
	}

	inline auto& GetCameraBuffer()
	{
		return m_cameraBuffer;
	}

};

NAMESPACE_END