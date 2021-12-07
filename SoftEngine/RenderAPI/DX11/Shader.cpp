#include "Shader.h"

#include "Renderer.h"
#include "DX11Global.h"

#include <Common.h>

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#include "DX11Global.h"
#include <iostream>

#include <File.h>
#include <Resource.h>


VertexShader::VertexShader(const std::wstring& path, uint32_t, void**) : IResource(path)
{
	ID3DBlob* errblob = nullptr;

	auto fullPath = CombinePath(Resource::ShaderDirectory(), path) + L".hlsl";

	if (!SUCCEEDED(D3DCompileFromFile(fullPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", NULL, NULL, &m_blob, &errblob)))
	{
		if (errblob)
		{
			std::cout << (char*)errblob->GetBufferPointer() << '\n';
			errblob->Release();
		}
		
		Throw(L"Can not compile vertex shader.");
	}

	if (errblob)
	{
		errblob->Release();
	}

	if (!SUCCEEDED(DX11Global::renderer->m_d3dDevice->CreateVertexShader(
		m_blob->GetBufferPointer(), m_blob->GetBufferSize(), NULL, &m_vs)))
	{
		Throw(L"CreateVertexShader() failed.");
	}
}

VertexShader::~VertexShader()
{
	m_vs->Release();
	if (m_inputLayout) m_inputLayout->Release();
	if (m_blob) m_blob->Release();
}


void VertexShader::SetInputLayout(const char* source)
{
	if (m_inputLayout) return;

	std::string src = source;

	auto begin = src.find('{');

	auto end = src.find('}');

	auto i = begin;

	std::vector<D3D11_INPUT_ELEMENT_DESC> layouts;
	std::vector<std::string> semantics;

	std::vector<std::string> params;

	UINT count = 0;
	uint32_t inputSlot = 0;

	bool prePerVertex = true;

	uint32_t extraData = 0;

	while (i < end)
	{
		auto b = src.find_first_not_of("\n\t ", i + 1);
		auto e = src.find('#', b);

		if (e == std::wstring::npos) break;

		std::string line = src.substr(b, e - b);

		if (line.empty()) Throw(L"SetInputLayout() on invalid source.");

		while (line.back() == ' ' || line.back() == '\t')
		{
			line.pop_back();
		}

		std::string type = line.substr(0, line.find(' '));
		std::string ext = line.substr(line.find_first_not_of("\t ", line.find(';') + 1));

		StringSplit(ext, ",", params);

		uint32_t semanticIndex = 0;

		if (std::isdigit(params[0].back()))
		{
			semanticIndex = params[0].back() - '0';
			params[0].pop_back();
		}

		semantics.push_back(params[0]);

		ToLower(type);

		bool perVertex = params[1].find("PER_VERTEX") != std::string::npos;
		auto dataType = perVertex ? D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA;
		//auto inputSlot = 0;

		if (perVertex != prePerVertex)
		{
			inputSlot++;
			prePerVertex = perVertex;
			if (params.size() != 3) Throw(L"SetInputLayout() on invalid source.");
			extraData = std::stoi(params[2]);
			count = 0;
		}

		if (type == "vec3")
		{
			layouts.push_back({ nullptr, semanticIndex, DXGI_FORMAT_R32G32B32_FLOAT, inputSlot, count, dataType, extraData });
			count += 12;
		}
		else if (type == "vec2")
		{
			layouts.push_back({ nullptr, semanticIndex, DXGI_FORMAT_R32G32_FLOAT, inputSlot, count, dataType, extraData });
			count += 8;
		}
		else if (type == "vec4")
		{
			layouts.push_back({ nullptr, semanticIndex, DXGI_FORMAT_R32G32B32A32_FLOAT, inputSlot, count, dataType, extraData });
			count += 16;
		}
		else if (type[0] == 'v' || type[1] == 'e' || type[2] == 'c')
		{
			auto num = type[3] - '0';
			auto priType = type.back();
			auto sign = *(&type.back() - 1);

			DXGI_FORMAT dxgiType = DXGI_FORMAT_UNKNOWN;
			UINT _size = 0;

			if (priType == 's' && num == 4)
			{
				dxgiType = sign == 'u' ? DXGI_FORMAT_R16G16B16A16_UINT : DXGI_FORMAT_R16G16B16A16_SINT;
				_size = 8;
			}
			else if (priType == 'i' && num == 4)
			{
				dxgiType = sign == 'u' ? DXGI_FORMAT_R32G32B32A32_UINT : DXGI_FORMAT_R32G32B32A32_SINT;
				_size = 16;
			}
			else
			{
				goto ThrowLabel;
			}
			layouts.push_back({ nullptr, semanticIndex, dxgiType, inputSlot, count, dataType, extraData });
			count += _size;
		}
		else
		{
		ThrowLabel:
			Throw(L"SetInputLayout() on invalid source.");
		}
		i = e + 1;
	}
	auto device = DX11Global::renderer->m_d3dDevice;

	for (size_t j = 0; j < layouts.size(); j++)
	{
		layouts[j].SemanticName = semantics[j].c_str();
	}

	if (FAILED(device->CreateInputLayout(
					&layouts[0], layouts.size(),
					m_blob->GetBufferPointer(),
					m_blob->GetBufferSize(),
					&m_inputLayout)))
	{
		Throw(L"CreateInputLayout() failed.");
	}

	m_blob->Release();
	m_blob = 0;

}

PixelShader::PixelShader(const std::wstring& path, uint32_t, void**) : IResource(path)
{
	ID3DBlob* errblob = nullptr;
	ID3DBlob* blob = nullptr;

	auto fullPath = CombinePath(Resource::ShaderDirectory(), path) + L".hlsl";

	if (!SUCCEEDED(D3DCompileFromFile(fullPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", NULL, NULL, &blob, &errblob)))
	{
		if (errblob)
		{
			std::cout << (char*)errblob->GetBufferPointer() << '\n';
			errblob->Release();
		}
		if (blob)
		{
			blob->Release();
		}

		Throw(L"Can not compile pixel shader.");
	}

	if (errblob)
	{
		errblob->Release();
	}

	if (!SUCCEEDED(DX11Global::renderer->m_d3dDevice->CreatePixelShader(
		blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &m_ps)))
	{
		if (blob)
		{
			blob->Release();
		}
		Throw(L"CreateVertexShader() failed.");
	}

	if (blob)
	{
		blob->Release();
	}
}

PixelShader::~PixelShader()
{
	m_ps->Release();
}


GeometryShader::GeometryShader(const std::wstring& path, uint32_t, void**) : IResource(path)
{
	ID3DBlob* errblob = nullptr;
	ID3DBlob* blob = nullptr;

	auto fullPath = CombinePath(Resource::ShaderDirectory(), path) + L".hlsl";

	if (!SUCCEEDED(D3DCompileFromFile(fullPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "gs_5_0", NULL, NULL, &blob, &errblob)))
	{
		if (errblob)
		{
			std::cout << (char*)errblob->GetBufferPointer() << '\n';
			errblob->Release();
		}
		if (blob)
		{
			blob->Release();
		}

		Throw(L"Can not compile vertex shader.");
	}

	if (errblob)
	{
		errblob->Release();
	}

	if (!SUCCEEDED(DX11Global::renderer->m_d3dDevice->CreateGeometryShader(
		blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &m_gs)))
	{
		if (blob)
		{
			blob->Release();
		}
		Throw(L"CreateVertexShader() failed.");
	}

	if (blob)
	{
		blob->Release();
	}
}

GeometryShader::~GeometryShader()
{
	m_gs->Release();
}

