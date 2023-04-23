#pragma once
#include "TypeDef.h"

#include "Graphics/DebugGraphics.h"

#include "DX12RenderParams.h"

NAMESPACE_DX12_BEGIN

class DX12DebugGraphics : public DebugGraphics
{
public:
	friend class DX12Graphics;

	// brrow from DX12Graphics
	DX12Graphics* m_graphics;
	ID3D12Device2* m_device;

	DX12RenderParams m_cubeParams;
	DX12RenderParams m_cubeWireframeParams;

	ComPtr<ID3D12PipelineState> m_cubePSO;
	ComPtr<ID3D12PipelineState> m_cubeWireframePSO;

public:
	DX12DebugGraphics(DX12Graphics* graphics);
	~DX12DebugGraphics();

private:
	void InitCubeRenderer();

	void BeginFrame();
	void EndFrame();

	void BeginCamera();
	void EndCamera();

	void RenderCubes(DX12RenderParams& params, ID3D12PipelineState* state);

public:
	// Inherited via DebugGraphics
	virtual void DrawAABox(const AABox& aaBox, const Vec4& color, bool wireframe) override;
	virtual void DrawCube(const Box& box, const Vec4& color, bool wireframe) override;
	virtual void DrawCube(const Mat4& transform, const Vec4& color, bool wireframe) override;
	virtual void DrawSphere(const Sphere& sphere, const Vec4& color, bool wireframe) override;
};


NAMESPACE_DX12_END