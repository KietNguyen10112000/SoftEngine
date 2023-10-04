#include "Texture2D.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"

NAMESPACE_BEGIN

const char* Texture2D::CACHE_EXTENSION = ".texture2d";

Texture2D::Texture2D(String path) : ResourceBase(path)
{
	assert(FileSystem::Get()->IsFileExist(path.c_str()));

	ByteStream stream;
	if (FileSystem::Get()->IsFileChanged(path.c_str()) || !FileSystem::Get()->ReadStream((path + CACHE_EXTENSION).c_str(), &stream))
	{
		CreateCache(path);
		return;
	}

	LoadCache(&stream);
}

void Texture2D::LoadCache(ByteStream* stream)
{
	byte* imageMips;
	uint32_t widths[32], heights[32], channels, mipLevel;
	size_t imageLen;
	ReadCache(stream, &imageMips, &imageLen, widths, heights, &channels, &mipLevel);

	GRAPHICS_SHADER_RESOURCE_DESC desc = {};
	desc.type = GRAPHICS_SHADER_RESOURCE_DESC::SHADER_RESOURCE_TYPE_TEXTURE2D;
	desc.texture2D.format = ConvertChannelsToGraphicsFormat(channels);
	desc.texture2D.width = widths[0];
	desc.texture2D.height = heights[0];
	desc.texture2D.mipLevels = mipLevel;
	
	Graphics::Get()->CreateShaderResources(1, &desc, &m_shaderResource);

	TEXTURE2D_REGION region = {};
	region.x = 0;
	region.y = 0;
	region.pixelStride = channels;
	auto buf = imageMips;
	for (uint32_t i = 0; i < mipLevel; i++)
	{
		region.mipLevel = i;
		region.width = widths[i];
		region.height = heights[i];
		m_shaderResource->UpdateTexture2D(buf, imageLen, region, i == mipLevel - 1);

		buf += region.width * region.height * channels;
	}

	rheap::free(imageMips);
}

void Texture2D::CreateCache(String path)
{
	byte* buffer;
	size_t fileSize;
	FileUtils::ReadFile(path, buffer, fileSize);

	int width, height, numComponents;
	auto imaData = stbi_load_from_memory(buffer, (int)fileSize, &width, &height, &numComponents, 4);

	WriteCache(path + CACHE_EXTENSION, imaData, width, height, numComponents, -1);

	stbi_image_free(imaData);

	FileUtils::FreeBuffer(buffer);
}

void Texture2D::WriteCache(String path, byte* data, uint32_t width, uint32_t height, uint32_t channels, uint32_t mipLevel)
{
	if (mipLevel == -1)
	{
		mipLevel = std::log2(std::max(width, height));
	}

	size_t mipSizes[32] = {};
	int mipCompressedSize[32] = {};
	stbi_uc* mipsCompressedData[32] = {};

	size_t imageLen = width * height * channels;
	size_t bufferSize = std::max(64 * KB, (size_t)std::ceil(imageLen * 1.6f));
	byte* buffer = (byte*)rheap::malloc(bufferSize);

	// 0th mip level
	std::memcpy(buffer, data, imageLen);
	mipSizes[0] = imageLen;
	
	byte* buf = buffer;
	size_t totalSize = imageLen;
	for (uint32_t i = 1; i < mipLevel; i++)
	{
		auto prevW = width / (1 << (i - 1));
		auto prevH = height / (1 << (i - 1));
		auto w = width / (1 << i);
		auto h = height / (1 << i);

		auto prevImageBuf = buf;
		buf += imageLen;
		imageLen = w * h * channels;

		stbir_resize_uint8(prevImageBuf, prevW, prevH, channels * w, buf, w, h, channels, channels);

		totalSize += imageLen;

		mipSizes[i] = imageLen;
	}

	buf = buffer;
	for (uint32_t i = 0; i < mipLevel; i++)
	{
		auto w = width / (1 << i);
		auto h = height / (1 << i);

		mipsCompressedData[i] = stbi_write_png_to_mem(buf, channels * w, w, h, channels, &mipCompressedSize[i]);

		buf += mipSizes[i];
	}

	ByteStream stream;
	stream.Resize(bufferSize + sizeof(size_t) * 4);
	
	stream.Put(mipLevel);
	stream.Put(channels);
	uint32_t sumMipCompressedSize = 0;
	for (uint32_t i = 0; i < mipLevel; i++)
	{
		stream.Put(mipCompressedSize[i]);
		sumMipCompressedSize += mipCompressedSize[i];
	}

	buf = buffer;
	stream.Put(sumMipCompressedSize);
	for (uint32_t i = 0; i < mipLevel; i++)
	{
		stream.PutBuffer(mipsCompressedData[i], mipCompressedSize[i]);
		buf += mipSizes[i];

		stbi_image_free(mipsCompressedData[i]);
	}

	FileSystem::Get()->WriteStream((path).c_str(), &stream);

	rheap::free(buffer);
}

void Texture2D::ReadCache(ByteStream* _stream, byte** output, size_t* outputSize, uint32_t* pWidths, uint32_t* pHeights, uint32_t* pChannels, uint32_t* pMipLevel)
{
	//auto stream = ByteStreamRead::From(_stream);

	auto& stream = *_stream;

	int mipsW[32] = {};
	int mipsH[32] = {};
	int mipCompressedSize[32] = {};
	stbi_uc* mipsData[32] = {};
	uint32_t mipsSizes[32] = {};
	uint32_t sumMipCompressedSize = 0;

	stream.Pick(*pMipLevel);
	stream.Pick(*pChannels);

	auto mipLevel = *pMipLevel;
	auto channels = *pChannels;
	for (uint32_t i = 0; i < mipLevel; i++)
	{
		stream.Pick(mipCompressedSize[i]);
	}

	stream.Pick(sumMipCompressedSize);
	size_t bufferSize = sumMipCompressedSize;
	auto buffer = stream.SkipBuffer(sumMipCompressedSize);
	size_t outputBufferSize = 0;

	auto buf = buffer;
	for (uint32_t i = 0; i < mipLevel; i++)
	{
		int comp = 0;
		mipsData[i] = stbi_load_from_memory(buf, mipCompressedSize[i], &mipsW[i], &mipsH[i], &comp, channels);
		assert(comp == channels);

		buf += mipCompressedSize[i];

		mipsSizes[i] = mipsW[i] * mipsH[i] * channels;
		outputBufferSize += mipsSizes[i];
	}

	auto outputBuffer = (byte*)rheap::malloc(std::max(64 * KB, outputBufferSize));
	buf = outputBuffer;
	for (uint32_t i = 0; i < mipLevel; i++)
	{
		std::memcpy(buf, mipsData[i], mipsSizes[i]);
		buf += mipsSizes[i];

		stbi_image_free(mipsData[i]);
	}

	*output = outputBuffer;
	for (uint32_t i = 0; i < mipLevel; i++)
	{
		pWidths[i] = mipsW[i];
		pHeights[i] = mipsH[i];
	}

	*outputSize = outputBufferSize;
}

GRAPHICS_DATA_FORMAT::FORMAT Texture2D::ConvertChannelsToGraphicsFormat(uint32_t channels)
{
	switch (channels)
	{
	case 1:
		return GRAPHICS_DATA_FORMAT::FORMAT_R8_UNORM;
	case 3:
		return GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8_UNORM;
	case 4:
		return GRAPHICS_DATA_FORMAT::FORMAT_R8G8B8A8_UNORM;
	default:
		assert(0);
		break;
	}
}

NAMESPACE_END