#pragma once

#include "TypeDef.h"

#include "Core/Memory/MemoryKeeper.h"
#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_DX12_BEGIN

class DX12ResourceUploader
{
public:
	constexpr static size_t DEFAULT_BUFFER_SIZE = 64 * MB;

private:
	friend class DX12Graphics;

	struct UploadingBlock
	{
		MemoryKeeper::Block* block = 0;
		uint64_t fenceValue = 0;
	};

	MemoryKeeper m_memoryKeeper;
	size_t m_bufferSize = DEFAULT_BUFFER_SIZE;

	std::list<UploadingBlock, STLNodeGlobalAllocator<UploadingBlock>> m_uploadingBlocks;

	ComPtr<ID3D12Resource> m_gpuBufferResource;
	void* m_gpuBuffer;

public:
	~DX12ResourceUploader();

private:
	void Initialize();
	void GrowthBuffer(size_t minSize);
	void InitBuffer(size_t size);

public:
	void UploadBuffer(ID3D12Resource* destResource, size_t destOffset, void* buffer, size_t bufferSize, bool endUploadChain);
	void UploadTexture2D(
		ID3D12Resource* destResource,
		uint32_t destOffsetX,
		uint32_t destOffsetY,
		void* srcBuffer,
		uint32_t srcWidth,
		uint32_t srcheight,
		uint32_t pixelStride,
		bool endUploadChain
	);


};

NAMESPACE_DX12_END