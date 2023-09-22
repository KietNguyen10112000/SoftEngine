#include "DX12ResourceUploader.h"

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

void DX12ResourceUploader::UploadBuffer(ID3D12Resource* destResource, size_t destOffset, void* buffer, size_t size, bool endUploadChain)
{
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

	std::memcpy(block->mem, buffer, size);
	cmdList->CopyBufferRegion(destResource, destOffset, m_gpuBufferResource.Get(), block->mem - (byte*)m_gpuBuffer, size);

	if (endUploadChain)
		dx12->ExecuteCurrentCmdList();
}

void DX12ResourceUploader::UploadTexture2D(
	ID3D12Resource* destResource,
	uint32_t destOffsetX,
	uint32_t destOffsetY,
	void* srcBuffer,
	uint32_t srcWidth,
	uint32_t srcheight,
	uint32_t pixelStride,
	bool endUploadChain
) {
	assert(0);

}


NAMESPACE_DX12_END