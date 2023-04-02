#pragma once
#include "TypeDef.h"

#include "Graphics/GraphicsCommandList.h" 

NAMESPACE_DX12_BEGIN

class DX12Graphics;

class DX12GraphicsCommandList : public GraphicsCommandList
{
public:
	DX12Graphics* m_graphics;
	ID3D12GraphicsCommandList* m_commandList;

public:
	DX12GraphicsCommandList();
	~DX12GraphicsCommandList();

public:
	void Init(DX12Graphics* graphics);

public:
	virtual void ClearScreen(const Vec4& color) override;

};

NAMESPACE_DX12_END