#include "DX11Resource.h"

#include <Common.h>

#include <DirectXTex.h>

#include <File.h>

#include "DX11Global.h"
#include "Renderer.h"

Texture2D::Texture2D(const std::wstring& path, uint32_t numArg, void** args) : IResource(path)
{
    ID3D11Device* deviceT = DX11Global::renderer->m_d3dDevice;
    ID3D11DeviceContext* deviceContextT = DX11Global::renderer->m_d3dDeviceContext;

    HRESULT hr = S_OK;
    DirectX::ScratchImage mips_image;

    hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, mips_image);

    if (FAILED(hr))
    {
        Throw(L"LoadFromWICFile() failed.");
    }

    m_width = mips_image.GetMetadata().width;
    m_height = mips_image.GetMetadata().height;

    auto metadata = mips_image.GetMetadata();

    size_t rowPitch = 0, slicePitch = 0;

    DirectX::ComputePitch(metadata.format, metadata.width, metadata.height, rowPitch, slicePitch);

    if (numArg != 0)
    {
        size_t flag = *(size_t*)args[0];

        if ((flag & DISCARD_SRGB) != 0)
        {
            switch (metadata.format)
            {
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                metadata.format = DXGI_FORMAT_B8G8R8A8_UNORM;
                break;
            default:
                break;
            }
        }  
    }
    

    D3D11_TEXTURE2D_DESC textureDesc = {};

    textureDesc.Height = metadata.height;
    textureDesc.Width = metadata.width;
    textureDesc.MipLevels = 0;
    textureDesc.ArraySize = 1;
    textureDesc.Format = metadata.format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    hr = deviceT->CreateTexture2D(&textureDesc, 0, &m_texture);

    if (FAILED(hr))
    {
        Throw("CreateTexture2D() failed.");
    }

    deviceContextT->UpdateSubresource(m_texture, 0, NULL, mips_image.GetPixels(), rowPitch, 0);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = metadata.format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;

    hr = deviceT->CreateShaderResourceView(m_texture, &srvDesc, &m_shaderRView);
    if (FAILED(hr))
    {
        Throw("CreateShaderResourceView() failed.");
    }
    deviceContextT->GenerateMips(m_shaderRView);
}

Texture2D::~Texture2D()
{
    if (m_texture)
    {
        m_texture->Release();
        m_texture = nullptr;
    }
    if (m_shaderRView)
    {
        m_shaderRView->Release();
        m_shaderRView = nullptr;
    }
}

TextureCube::TextureCube(const std::wstring& path, uint32_t numArg, void** args) : IResource(path)
{
    auto ext = GetExtension(path);

    if (numArg != 0)
    {
        bool useMT = false;
        bool from2DTexture = true;
        for (size_t i = 0; i < numArg; i++)
        {
            if ((uint32_t)(args[i]) == USE_MULTI_THREAD_LOADING)
            {
                useMT = true;
            }
            else if ((uint32_t)(args[i]) == FROM_2D_TEXTURE)
            {
                useMT = true;
            }
        }

        if (from2DTexture)
        {
            LoadCubeMap(path, useMT);
            return;
        }

        if (ext == L"dds")
        {
            LoadDDSCubeMap(path);
            return;
        }
        LoadCubeMap(path, useMT);
    }
    else
    {
        if (ext == L"dds")
        {
            LoadDDSCubeMap(path);
            return;
        }
        LoadCubeMap(path);
    }
}

TextureCube::~TextureCube()
{
    if (m_texture)
    {
        m_texture->Release();
        m_texture = nullptr;
    }
    if (m_shaderRView)
    {
        m_shaderRView->Release();
        m_shaderRView = nullptr;
    }
}

void TextureCube::LoadCubeMap(const std::wstring& path, bool multiThreading)
{
    ID3D11Device* deviceT = DX11Global::renderer->m_d3dDevice;
    ID3D11DeviceContext* deviceContextT = DX11Global::renderer->m_d3dDeviceContext;

    HRESULT hr = S_OK;
    DirectX::ScratchImage image_data;

    hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image_data);

    if (FAILED(hr)) exit(-1);

    unsigned int imgW = image_data.GetMetadata().width;
    unsigned int imgH = image_data.GetMetadata().height;

    unsigned int subImgW = imgW / 4;
    unsigned int subImgH = imgH / 3;

    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width = subImgW;
    texDesc.Height = subImgH;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 6;
    texDesc.Format = image_data.GetMetadata().format;
    texDesc.CPUAccessFlags = 0;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
    SMViewDesc.Format = texDesc.Format;
    SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    SMViewDesc.TextureCube.MipLevels = texDesc.MipLevels;
    SMViewDesc.TextureCube.MostDetailedMip = 0;

    D3D11_SUBRESOURCE_DATA pData[6];

    struct vector4uc
    {
        unsigned char a[4];
    };

    std::vector<vector4uc> d[6]; // 6 images of type vector4uc = 4 * unsigned char

    int coord[6][2] =
    {
        {2 * imgW / 4, imgH / 3}, //phai
        {0, imgH / 3},            //trai
        {imgW / 4, 0},            //tren
        {imgW / 4, 2 * imgH / 3}, //duoi
        {imgW / 4, imgH / 3},     //truoc
        {3 * imgW / 4, imgH / 3}  //sau
    };

    m_dimension = imgW / 4;

    unsigned char* data = image_data.GetPixels();
    int dataSize = image_data.GetPixelsSize();

    for (int cubeMapFaceIndex = 0; cubeMapFaceIndex < 6; cubeMapFaceIndex++)
    {
        d[cubeMapFaceIndex].resize(subImgW * subImgH);
        pData[cubeMapFaceIndex].pSysMem = &d[cubeMapFaceIndex][0];
        pData[cubeMapFaceIndex].SysMemPitch = subImgW * 4;
        pData[cubeMapFaceIndex].SysMemSlicePitch = 0;
    }

    auto a = [&coord, &d, &data, imgW, subImgH, subImgW](int j)
    {
        for (size_t i = 0; i < subImgH; i++)
        {
            int index = (coord[j][1] + i) * imgW * 4 + coord[j][0] * 4;
            memcpy(&d[j][i * subImgH], &data[index], subImgW * 4);
        }
    };

    if (multiThreading)
    {
        std::vector<std::thread*> thrs(6);
        for (size_t j = 0; j < 6; j++)
        {
            thrs[j] = new std::thread(a, j);
        }

        for (size_t j = 0; j < 6; j++)
        {
            if (thrs[j]->joinable())
            {
                thrs[j]->join();
            }
        }
    }
    else
    {
        for (size_t j = 0; j < 6; j++)
        {
            a(j);
        }
    }

    hr = deviceT->CreateTexture2D(&texDesc, &pData[0], (ID3D11Texture2D**)&m_texture);
    if (FAILED(hr))
    {
        Throw(L"CreateTexture2D() failed.");
    }

    hr = deviceT->CreateShaderResourceView(m_texture, &SMViewDesc, &m_shaderRView);

    if (FAILED(hr))
    {
        Throw(L"CreateShaderResourceView() failed.");
    }
}

void TextureCube::LoadDDSCubeMap(const std::wstring& path)
{
    ID3D11Device* deviceT = DX11Global::renderer->m_d3dDevice;
    ID3D11DeviceContext* deviceContextT = DX11Global::renderer->m_d3dDeviceContext;

    HRESULT hr = S_OK;
    DirectX::ScratchImage image_data;

    hr = DirectX::LoadFromDDSFile(path.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image_data);

    if (FAILED(hr))
    {
        Throw(L"LoadFromDDSFile() failed.");
    }

    hr = DirectX::CreateTexture(deviceT, image_data.GetImages(),
        image_data.GetImageCount(), image_data.GetMetadata(), &m_texture);

    if (FAILED(hr))
    {
        Throw(L"CreateTexture() failed.");
    }

    //auto rowPitch = image_data.GetMetadata().width * 4 * sizeof(unsigned char);

    m_dimension = image_data.GetMetadata().width;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = image_data.GetMetadata().format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = (UINT)image_data.GetMetadata().mipLevels;

    hr = deviceT->CreateShaderResourceView(m_texture, &srvDesc, &m_shaderRView);

    if (FAILED(hr))
    {
        Throw(L"CreateShaderResourceView() failed.");
    }
}
