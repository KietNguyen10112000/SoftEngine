#pragma once
#include "TypeDef.h"

#include "Graphics/DebugGraphics.h"

#include "DX12RenderParams.h"

NAMESPACE_DX12_BEGIN

class DX12Graphics;

class DX12DebugGraphics : public DebugGraphics
{
public:
	// brrow from DX12Graphics
	DX12Graphics* m_graphics;
	ID3D12Device2* m_device;


	DX12RenderParams m_sceneParams;
	DX12RenderParams m_cubeParams;
	ComPtr<ID3D12PipelineState> m_cubePSO;

	size_t m_numBuiltInSRV = 2;
	size_t m_numBuiltInCBV = 2;
	D3D12_CPU_DESCRIPTOR_HANDLE m_builtInCBVs[2] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_builtInSRVs[2] = {};

public:
	DX12DebugGraphics(DX12Graphics* graphics);
	~DX12DebugGraphics();

private:
	void InitCubeRenderer();

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