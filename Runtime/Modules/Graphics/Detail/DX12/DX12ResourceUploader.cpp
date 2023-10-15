#include "DX12ResourceUploader.h"

#include "Resources/Texture2D.h"

NAMESPACE_DX12_BEGIN

DX12ResourceUploader::~DX12ResourceUploader()
{
}

void DX12ResourceUploader::Initialize()
{
	InitBuffer(DEFAULT_BUFFER_SIZE);
}

void DX12ResourceUploader::GrowthBuffer(size_t minSize)
{
	// not implemented yet!
	assert(0);
}

void DX12ResourceUploader::InitBuffer(size_t size)
{
	m_bufferSize = size;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	auto dx12device = DX12Graphics::GetDX12()->m_device.Get();
	ThrowIfFailed(dx12device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		0,
		IID_PPV_ARGS(&m_gpuBufferResource)
	));

	void* buffer;
	m_gpuBufferResource->Map(0, 0, &buffer);
	m_memoryKeeper.Reset(buffer, size);
	m_gpuBuffer = buffer;
}

MemoryKeeper::Block* DX12ResourceUploader::AllocateBlock(size_t size)
{
	size = MemoryUtils::Align<512>(size);
	if (size > m_bufferSize)
	{
		GrowthBuffer(size);
	}

	auto dx12 = DX12Graphics::GetDX12();
	auto cmdList = dx12->GetCmdList();
	auto cmdQueue = dx12->m_commandQueue.Get();

	auto block = m_memoryKeeper.Allocate(size);
	if (!block)
	{
		// wait for resource avaiable
		auto it = m_uploadingBlocks.begin();
		while (it != m_uploadingBlocks.end())
		{
			auto& uploadingblock = *it;
			dx12->WaitForDX12FenceValue(uploadingblock.fenceValue);

			m_memoryKeeper.Deallocate(uploadingblock.block);
			it = m_uploadingBlocks.erase(it);

			block = m_memoryKeeper.Allocate(size);

			if (block)
			{
				break;
			}
		}
	}

	m_uploadingBlocks.push_back({ block, dx12->GetCurrentDX12FenceValue() });

	return block;
}

void DX12ResourceUploader::UploadBuffer(ID3D12Resource* destResource, size_t destOffset, const void* buffer, size_t size, bool endUploadChain)
{
	auto dx12 = DX12Graphics::GetDX12();
	auto cmdList = dx12->GetCmdList();
	auto cmdQueue = dx12->m_commandQueue.Get();

	auto block = AllocateBlock(size);

	std::memcpy(block->mem, buffer, size);
	cmdList->CopyBufferRegion(destResource, destOffset, m_gpuBufferResource.Get(), block->mem - (byte*)m_gpuBuffer, size);

	if (endUploadChain)
		dx12->ExecuteCurrentCmdList();
}

void DX12ResourceUploader::Texture2DChannelsCopyFunc_1(void* dest, void* src, uint32_t width, uint32_t height, uint32_t destRowPitch)
{
	assert(0);
}

void DX12ResourceUploader::Texture2DChannelsCopyFunc_3(void* dest, void* src, uint32_t width, uint32_t height, uint32_t destRowPitch)
{
	auto srcRowData = (byte*)src;
	auto srcRowPitch = width * 3;
	auto destRowData = (byte*)dest;

	for (uint32_t y = 0; y < height; y++)
	{
		for (uint32_t x = 0; x < width; x++)
		{
			auto* srcPixelPtr = srcRowData + x * 3;
			auto* destPixelPtr = destRowData + x * 4;
			destPixelPtr[0] = srcPixelPtr[0];
			destPixelPtr[1] = srcPixelPtr[1];
			destPixelPtr[2] = srcPixelPtr[2];
			destPixelPtr[3] = 255;
		}
		
		srcRowData += srcRowPitch;
		destRowData += destRowPitch;
	}
}

void DX12ResourceUploader::Texture2DChannelsCopyFunc_4(void* dest, void* src, uint32_t width, uint32_t height, uint32_t destRowPitch)
{
	auto srcRowData = (byte*)src;
	auto srcRowPitch = width * 4;
	auto destRowData = (byte*)dest;

	for (uint32_t y = 0; y < height; y++)
	{
		std::memcpy(destRowData, srcRowData, srcRowPitch);

		srcRowData += srcRowPitch;
		destRowData += destRowPitch;
	}
}

void DX12ResourceUploader::UploadTexture2D(
	ID3D12Resource* destResource,
	uint32_t destOffsetX,
	uint32_t destOffsetY,
	const void* srcBuffer,
	uint32_t srcWidth,
	uint32_t srcHeight,
	uint32_t pixelStride,
	uint32_t mipLevel,
	bool endUploadChain
) {
	//assert(0);
	auto supportedStride = pixelStride == 3 ? 4 : pixelStride;

	uint32_t rowPitch = MemoryUtils::Align<256>(srcWidth * supportedStride);
	auto size = rowPitch * srcHeight;

	auto dx12 = DX12Graphics::GetDX12();
	auto cmdList = dx12->GetCmdList();
	auto cmdQueue = dx12->m_commandQueue.Get();

	// move buffer from ram to gpu shared memory (shared with cpu as virtual address)
	auto block = AllocateBlock(size);
	{
		assert(pixelStride <= 4);
		auto copyFunc = m_texture2dCopyFunc[pixelStride - 1];
		assert(copyFunc != nullptr);
		copyFunc(block->mem, (void*)srcBuffer, srcWidth, srcHeight, rowPitch);
	}

	// move gpu shared memory to gpu owned memory
	{
		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = m_gpuBufferResource.Get();
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint.Offset = block->mem - (byte*)m_gpuBuffer;
		src.PlacedFootprint.Footprint.Format = dx12utils::ConvertToDX12Format(Texture2D::ConvertChannelsToGraphicsFormat(pixelStride));
		src.PlacedFootprint.Footprint.Depth = 1;
		src.PlacedFootprint.Footprint.RowPitch = rowPitch;
		src.PlacedFootprint.Footprint.Width = srcWidth;
		src.PlacedFootprint.Footprint.Height = srcHeight;

		D3D12_TEXTURE_COPY_LOCATION dest = {};
		dest.pResource = destResource;
		dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dest.SubresourceIndex = mipLevel;

		cmdList->CopyTextureRegion(&dest, 0, 0, 0, &src, 0);
	}
	
	if (endUploadChain)
	{
		dx12->ExecuteCurrentCmdList();
	}
}


NAMESPACE_DX12_END