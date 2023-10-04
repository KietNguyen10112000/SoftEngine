#include "Detail.h"

#ifdef WIN32
#include "DX12/DX12Graphics.cpp"
#include "DX12/DX12RingBufferCommandList.cpp"
#include "DX12/DX12ResourceUploader.cpp"
#include "DX12/DX12GraphicsPipeline.cpp"
#include "DX12/DX12Resources.cpp"
#include "DX12/DX12ShaderResource.cpp"

#include "Libraries/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp"
#endif // WIN32
