#pragma once
#include "Core/TypeDef.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

enum class DEBUG_GRAPHICS_MODE
{
	WIRE_FRAME,
	SOLID
};

class GraphicsCommandList;

class DebugGraphics
{
public:
	virtual ~DebugGraphics() {};

public:
	virtual void SetCamera(const Mat4& view, const Mat4& proj) = 0;

	virtual void BeginDrawBatch(GraphicsCommandList* cmdList, DEBUG_GRAPHICS_MODE mode = DEBUG_GRAPHICS_MODE::SOLID) = 0;
	virtual void EndDrawBatch() = 0;

	virtual void DrawAABox(const AABox& aaBox, const Vec4& color) = 0;
	virtual void DrawCube(const Box& box, const Vec4& color) = 0;
	virtual void DrawSphere(const Sphere& sphere, const Vec4& color) = 0;

};

NAMESPACE_END