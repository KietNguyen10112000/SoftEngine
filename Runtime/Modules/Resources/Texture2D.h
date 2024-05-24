#pragma once
#include "Resource.h"

#include "Graphics/Graphics.h"
#include "FileSystem/FileSystem.h"

NAMESPACE_BEGIN

class API Texture2D : public ResourceBase
{
public:
	static const char* CACHE_EXTENSION;

	static const char* DEFAULT_FILE;

	SharedPtr<GraphicsShaderResource> m_shaderResource;

	uint32_t m_width = 0;
	uint32_t m_height = 0;

protected:
	virtual int Load(const String& path) override;

private:
	void LoadCache(ByteStream* stream);
	void CreateCache(String path);

public:
	static void WriteCache(String path, byte* data, uint32_t width, uint32_t height, uint32_t channels, uint32_t mipLevel);
	static void ReadCache(ByteStream* stream, byte** output, size_t* outputSize, uint32_t* pWidths, uint32_t* pHeights, uint32_t* pChannels, uint32_t* pMipLevel);
	static GRAPHICS_DATA_FORMAT::FORMAT ConvertChannelsToGraphicsFormat(uint32_t channels);

	void* GetNativeHandle();

	inline const uint32_t& Width() const
	{
		return m_width;
	}

	inline const uint32_t& Height() const
	{
		return m_height;
	}

	inline auto& GetGraphicsShaderResource()
	{
		return m_shaderResource;
	}
};

NAMESPACE_END