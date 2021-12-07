#pragma once

#include <IResource.h>

#include <d3d11.h>

#include <Math\Math.h>

//args[0] is Texture2D::FLAG
class Texture2D : public IResource
{
protected:
	ID3D11Texture2D* m_texture = nullptr;
	ID3D11ShaderResourceView* m_shaderRView = nullptr;
	int m_width = 0;
	int m_height = 0;

public:
	enum FLAG
	{
		DISCARD_SRGB = 1
	};

public:
	Texture2D(const std::wstring& path, uint32_t numArg, void** args);
	~Texture2D();

public:
	inline auto& GetNativeSRVHandle() const { return m_shaderRView; };
	inline auto& GetNativeTextureHandle() const { return m_texture; };

	inline auto& GetNativeHandle() const { return m_shaderRView; };

public:
	inline auto Width() { return m_width; };
	inline auto Height() { return m_height; };

};


//example args = { TextureCube::OPTION::USE_MULTI_THREAD_LOADING, TextureCube::OPTION::FROM_2D_TEXTURE }
class TextureCube : public IResource
{
protected:
	ID3D11Resource* m_texture = nullptr;
	ID3D11ShaderResourceView* m_shaderRView = nullptr;
	uint32_t m_dimension = 0;

public:
	enum OPTION
	{
		USE_MULTI_THREAD_LOADING,
		FROM_2D_TEXTURE
	};

public:
	TextureCube(const std::wstring& path, uint32_t numArg, void** args);
	~TextureCube();

private:
	void LoadCubeMap(const std::wstring& path, bool multiThreading = false);
	void LoadDDSCubeMap(const std::wstring& path);

public:
	inline auto& GetNativeSRVHandle() const { return m_shaderRView; };
	inline auto& GetNativeTextureHandle() const { return m_texture; };

	inline auto& GetNativeHandle() const { return m_shaderRView; };

public:
	//width, height, depth is same
	inline auto& Dimension() const { return m_dimension; };

};