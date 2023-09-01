#pragma once
#include "Core/TypeDef.h"

#include "Core/Pattern/Singleton.h"

NAMESPACE_BEGIN

enum class GRAPHICS_BACKEND_API
{
	DX12
};

class DebugGraphics;
class RenderingSystem;
class Camera;

class API Graphics : public Singleton<Graphics>
{
public:
	DebugGraphics* m_debugGraphics = nullptr;

	RenderingSystem* m_bindedRdSys = nullptr;

public:
	virtual ~Graphics() {};

public:
	static int Initilize(void* windowNativeHandle, GRAPHICS_BACKEND_API backendAPI);
	static void Finalize();

public:
	

public:
	inline auto GetDebugGraphics()
	{
		return m_debugGraphics;
	}

	inline void Bind(RenderingSystem* sys)
	{
		m_bindedRdSys = sys;
	}

	inline auto GetRenderingSystem()
	{
		return m_bindedRdSys;
	}

};

NAMESPACE_END