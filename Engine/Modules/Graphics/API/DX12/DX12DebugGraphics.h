#pragma once
#include "TypeDef.h"

#include "Graphics/DebugGraphics.h"

NAMESPACE_DX12_BEGIN

class DX12Graphics;

class DX12DebugGraphics : public DebugGraphics
{
public:
	// brrow from DX12Graphics
	DX12Graphics* m_graphics;
	ID3D12Device2* m_device;


public:
	DX12DebugGraphics(DX12Graphics* graphics);
	~DX12DebugGraphics();

private:
	void InitCubeInstancingRenderer();

public:
	// Inherited via DebugGraphics
	virtual void SetCamera(const Mat4& view, const Mat4& proj) override;
	virtual void BeginDrawBatch(GraphicsCommandList* cmdList, DEBUG_GRAPHICS_MODE mode) override;
	virtual void EndDrawBatch() override;
	virtual void DrawAABox(const AABox& aaBox, const Vec4& color) override;
	virtual void DrawCube(const Box& box, const Vec4& color) override;
	virtual void DrawSphere(const Sphere& sphere, const Vec4& color) override;
};


NAMESPACE_DX12_END