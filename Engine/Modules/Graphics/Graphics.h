#pragma once
#include "Core/TypeDef.h"

#include "Core/Pattern/Singleton.h"

NAMESPACE_BEGIN

enum class GRAPHICS_BACKEND_API
{
	DX12
};

class DebugGraphics;

class GraphicsCommandList;

class Graphics : public Singleton<Graphics>
{
public:
	DebugGraphics* m_debugGraphics = nullptr;

public:
	virtual ~Graphics() {};

public:
	static int Initilize(void* windowNativeHandle, GRAPHICS_BACKEND_API backendAPI);
	static void Finalize();

public:
	// get a new command list to execute command
	virtual void BeginCommandList(GraphicsCommandList** cmdList) = 0;

	// close command list and flush to command queue
	virtual void EndCommandList(GraphicsCommandList** cmdList) = 0;

	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;

public:
	inline auto GetDebugGraphics()
	{
		return m_debugGraphics;
	}

};

NAMESPACE_END