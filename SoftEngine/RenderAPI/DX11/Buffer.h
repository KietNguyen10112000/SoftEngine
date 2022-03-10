#pragma once
#include <d3d11.h>

class VertexBuffer
{
private:
	ID3D11Buffer* m_buffer = nullptr;
	int m_vertexCount = 0;
	unsigned int m_stride = 0;
	unsigned int m_offset = 0;

public:
	enum FLAG
	{
		STATIC	= 1,
		DYNAMIC = 2
	};

public:
	VertexBuffer(const void* data, int numDataElm, int dataElmSize, size_t flag = 1);
	~VertexBuffer();

public:
	//size in byte
	void Update(const void* data, size_t size);

public:
	//recommend not use this section in engine
	inline auto& GetNativeHandle() const { return m_buffer; };
	inline auto& Stride() const { return m_stride; };
	inline auto& Offset() const { return m_offset; };
	inline auto& Count() { return m_vertexCount; };

};

class IndexBuffer
{
private:
	ID3D11Buffer* m_buffer = nullptr;
	DXGI_FORMAT m_format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
	unsigned int m_indexCount = 0;

public:
	IndexBuffer(const void* data, int numDataElm, int dataElmSize);
	~IndexBuffer();

public:
	void Update(const void* data, size_t size);

public:
	//recommend not use this section in engine
	inline auto& GetNativeHandle() const { return m_buffer; };
	inline auto& GetNativeFormat() const { return m_format; };
	inline auto& Count() const { return m_indexCount; };

};


//recommend use ShaderVar instead
class ConstantBuffer
{
private:
	ID3D11Buffer* m_buffer = nullptr;

public:
	ConstantBuffer(const void* data, int numDataElm, int dataElmSize);
	~ConstantBuffer();

public:
	void Update(const void* data);

public:
	//recommend not use this section in engine
	inline auto& GetNativeHandle() const { return m_buffer; };

};