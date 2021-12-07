#pragma once

#include <IResource.h>

#include <d3d11.h>

//if OpenGL
//typedef uint32_t VertexShader;
//typedef uint32_t PixelShader;
//typedef ID3D11VertexShader VertexShader;

//fragment shader in OpenGL
//typedef ID3D11PixelShader PixelShader;
//typedef ID3D11HullShader HullShader;
//typedef ID3D11GeometryShader GeometryShader;

class VertexShader : public IResource
{
private:
	ID3DBlob* m_blob = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11VertexShader* m_vs = nullptr;

public:
	VertexShader(const std::wstring& path, uint32_t, void**);
	~VertexShader();

public:
	/*eg source:
	struct Vertex
	{
		Vec3 pos; POSITION, PER_VERTEX #
		Vec2 textCoord; TEXCOORD, PER_VERTEX #
	};
	*/
	void SetInputLayout(const char* source);

public:
	inline auto& GetNativeHandle() { return m_vs; };
	inline auto& GetNativeLayoutHandle() { return m_inputLayout; };

};

class PixelShader : public IResource
{
private:
	ID3D11PixelShader* m_ps = nullptr;

public:
	PixelShader(const std::wstring& path, uint32_t, void**);
	~PixelShader();

public:
	inline auto& GetNativeHandle() { return m_ps; };

};

class GeometryShader : public IResource
{
private:
	ID3D11GeometryShader* m_gs = nullptr;

public:
	GeometryShader(const std::wstring& path, uint32_t, void**);
	~GeometryShader();

public:
	inline auto& GetNativeHandle() { return m_gs; };
};

class HullShader : public IResource
{
private:
	ID3D11HullShader* m_hs = nullptr;

public:
	HullShader(const std::wstring& path, uint32_t, void**);
	~HullShader();

public:
	inline auto& GetNativeHandle() { return m_hs; };

};

class DomainShader : public IResource
{
private:
	ID3D11DomainShader* m_ds = nullptr;

public:
	DomainShader(const std::wstring& path, uint32_t, void**);
	~DomainShader();

public:
	inline auto& GetNativeHandle() { return m_ds; };

};