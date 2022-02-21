#pragma once

#include <IObject.h>

class TextureCube;
class VertexBuffer;
class IndexBuffer;

//example args = { TextureCube::OPTION::USE_MULTI_THREADING, TextureCube::OPTION::FROM_2D_TEXTURE }
class SkyCube : public IRenderableObject
{
protected:
	TextureCube* m_cubeTexture = nullptr;
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;

public:
	SkyCube(const std::wstring& path, uint32_t numArg = 0, void** args = 0);
	~SkyCube();

public:
	// Inherited via IRenderableObject
	virtual void Update(Engine* engine) override;

	virtual void Render(IRenderer* renderer) override;

};