#pragma once
#include "Resource.h"

#include "Graphics/Graphics.h"
#include "FileSystem/FileSystem.h"

NAMESPACE_BEGIN

class Texture2D : public ResourceBase
{
public:
	static const char* CACHE_EXTENSION;

	SharedPtr<GraphicsShaderResource> m_shaderResource;

public:
	Texture2D(String path);

private:
	void LoadCache(ByteStream* stream);
	void CreateCache(String path);

public:
	static void WriteCache(String path, byte* data, uint32_t width, uint32_t height, uint32_t channels, uint32_t mipLevel);
	static void ReadCache(ByteStream* stream, byte** output, size_t* outputSize, uint32_t* pWidths, uint32_t* pHeights, uint32_t* pChannels, uint32_t* pMipLevel);
	static GRAPHICS_DATA_FORMAT::FORMAT ConvertChannelsToGraphicsFormat(uint32_t channels);


	inline auto& GetGraphicsShaderResource()
	{
		return m_shaderResource;
	}
};

NAMESPACE_END