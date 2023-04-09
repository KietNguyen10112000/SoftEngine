#include "DX12DebugGraphics.h"

NAMESPACE_DX12_BEGIN

void DX12DebugGraphics::SetCamera(const Mat4& view, const Mat4& proj)
{
}

void DX12DebugGraphics::BeginDrawBatch(GraphicsCommandList* cmdList, DEBUG_GRAPHICS_MODE mode)
{
}

void DX12DebugGraphics::EndDrawBatch()
{
}

void DX12DebugGraphics::DrawAABox(const AABox& aaBox, const Vec4& color)
{
}

void DX12DebugGraphics::DrawCube(const Box& box, const Vec4& color)
{
}

void DX12DebugGraphics::DrawSphere(const Sphere& sphere, const Vec4& color)
{
}

NAMESPACE_DX12_END