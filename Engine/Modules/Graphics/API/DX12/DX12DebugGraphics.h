#pragma once
#include "TypeDef.h"

#include "Graphics/DebugGraphics.h"

NAMESPACE_DX12_BEGIN

class DX12DebugGraphics : public DebugGraphics
{
public:
	// Inherited via DebugGraphics
	virtual void SetCamera(const Mat4& view, const Mat4& proj) override;
	virtual void BeginDrawBatch(DEBUG_GRAPHICS_MODE mode) override;
	virtual void EndDrawBatch() override;
	virtual void DrawAABox(const AABox& aaBox, const Vec4& color) override;
	virtual void DrawCube(const Box& box, const Vec4& color) override;
	virtual void DrawSphere(const Sphere& sphere, const Vec4& color) override;
};


NAMESPACE_DX12_END