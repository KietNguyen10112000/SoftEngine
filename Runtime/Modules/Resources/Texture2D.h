#pragma once
#include "Resource.h"

#include "Graphics/Graphics.h"
#include "FileSystem/FileSystem.h"

NAMESPACE_BEGIN

class Texture2D : public ResourceBase
{
protected:
	SharedPtr<GraphicsShaderResource> m_shaderResource;

public:
	Texture2D(String path);

private:
	void ReadCache(ByteStream* stream);
	void CreateCache(String path);

	void MakeGraphics(byte* data, uint32_t width, uint32_t height, uint32_t mipLevel);

};

NAMESPACE_END