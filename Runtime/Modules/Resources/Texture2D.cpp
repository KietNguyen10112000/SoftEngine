#include "Texture2D.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize.h"

NAMESPACE_BEGIN

Texture2D::Texture2D(String path) : ResourceBase(path)
{
	ByteStream stream;
	if (!FileSystem::Get()->IsFileChanged(path.c_str()) && FileSystem::Get()->ReadStream((path + ".texturecache").c_str(), &stream))
	{
		// read cache files with mips
		ReadCache(&stream);
		return;
	}

	CreateCache(path);
}

void Texture2D::ReadCache(ByteStream* stream)
{
}

void Texture2D::CreateCache(String path)
{
	byte* buffer;
	size_t fileSize;
	FileUtils::ReadFile(path, buffer, fileSize);

	int width, height, numComponents;
	auto imaData = stbi_load_from_memory(buffer, (int)fileSize, &width, &height, &numComponents, 4);

	//ByteStream stream;
	//stream.Put(100);

	stbi_image_free(imaData);

	FileUtils::FreeBuffer(buffer);
}

void Texture2D::MakeGraphics(byte* data, uint32_t width, uint32_t height, uint32_t mipLevel)
{
}

NAMESPACE_END