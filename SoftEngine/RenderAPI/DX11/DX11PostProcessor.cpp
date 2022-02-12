#include "DX11PostProcessor.h"

#include <set>

#include "DX11Global.h"
#include "Renderer.h"

#include <Resource.h>

#include <Shader.h>

#include <Buffer.h>

#include <RenderPipeline.h>

#include "./LightSystem.h"

DX11PostProcessor::DX11PostProcessor(DX11Renderer* renderer)
{
	m_textures.resize(Texture2DData::TYPE::COUNT,
		std::map<
			Vec2,
			std::vector<CacheTexture2D>,
			bool(*)(const Vec2&, const Vec2&)
		>(___PostProcessor_Vec2_Compare)
	);

	InitReadOnlyTexture(renderer);

	m_screenVS = Resource::Get<VertexShader>(L"VSQuad");
}

DX11PostProcessor::~DX11PostProcessor()
{
	for (auto& texture : m_textures)
	{
		for (auto& t : texture)
		{
			for (auto& tt : t.second)
			{
				tt.rtv->Release();
				tt.srv->Release();
			}
			t.second.clear();
		}
		texture.clear();
	}

	for (auto ps : m_ps)
	{
		Resource::Release(&ps);
	}
	m_ps.clear();

	Resource::Release(&m_screenVS);
}

void DX11PostProcessor::InitReadOnlyTexture(DX11Renderer* _renderer)
{
	DeferredRenderer* renderer = dynamic_cast<DeferredRenderer*>(_renderer);

	m_readOnlyTextures.resize(6);

	m_readOnlyTextures[m_readOnlyTextureID[AvaiableTexture2D::POSITION_AND_SPECULAR]] = {
		renderer->m_rtv[DeferredRenderer::POSITION_SPECULAR],
		renderer->m_rtvShader[DeferredRenderer::POSITION_SPECULAR],
	};

	m_readOnlyTextures[m_readOnlyTextureID[AvaiableTexture2D::NORMAL_AND_SHININESS]] = {
		renderer->m_rtv[DeferredRenderer::NORMAL_SHININESS],
		renderer->m_rtvShader[DeferredRenderer::NORMAL_SHININESS],
	};

	m_readOnlyTextures[m_readOnlyTextureID[AvaiableTexture2D::COLOR]] = {
		renderer->m_rtv[DeferredRenderer::COLOR],
		renderer->m_rtvShader[DeferredRenderer::COLOR],
	};

	m_readOnlyTextures[m_readOnlyTextureID[AvaiableTexture2D::METALLIC_ROUGHNESS_AO]] = {
		renderer->m_rtv[DeferredRenderer::METALLIC_ROUGHNESS_AO],
		renderer->m_rtvShader[DeferredRenderer::METALLIC_ROUGHNESS_AO],
	};

	m_readOnlyTextures[m_readOnlyTextureID[AvaiableTexture2D::LIGHTED_SCENE]] = {
		renderer->m_rtv[DeferredRenderer::LIGHTED_SCENE],
		renderer->m_rtvShader[DeferredRenderer::LIGHTED_SCENE],
	};

	m_readOnlyTextures[m_readOnlyTextureID[AvaiableTexture2D::SCREEN_BUFFER]] = {
		renderer->m_mainRtv,
		renderer->m_mainRtvShader,
	};
}

DX11PostProcessor::ProcessChain* DX11PostProcessor::MakeProcessChain(const std::string& name)
{
	auto& ret = m_procChains[name];

	if (ret == 0)
	{
		auto temp = new DX11PostProcessChain();
		temp->m_owner = this;
		ret = temp;
	}

	return ret;
}

bool DX11PostProcessor::Run()
{
	if (!m_activeProcChain) return 0;

	DX11PostProcessChain* chain = (DX11PostProcessChain*)m_activeProcChain;

	auto ctx = DX11Global::renderer->m_d3dDeviceContext;

	DeferredRenderer* renderer = (DeferredRenderer*)(DX11Global::renderer);

	auto& RTVs = chain->m_RTVs;
	auto& SRVs = chain->m_SRVs;
	auto& VPs = chain->m_VPs;

	auto quad = renderer->m_screenQuad;

	auto screenVS = renderer->m_lastPresentRpl->GetVS();

	ctx->VSSetShader(screenVS->GetNativeHandle(), 0, 0);

	ctx->IASetInputLayout(screenVS->GetNativeLayoutHandle());
	ctx->IASetVertexBuffers(0, 1, &quad->GetNativeHandle(), &quad->Stride(), &quad->Offset());

	ID3D11ShaderResourceView* nullSRVs[15] = {};
	ID3D11RenderTargetView* nullRTVs[8] = {};

	ctx->OMSetRenderTargets(8, nullRTVs, 0);
	ctx->PSSetShaderResources(DX11PostProcessor::START_SRV_REGISTER, 15 - DX11PostProcessor::START_SRV_REGISTER, nullSRVs);

	ctx->PSSetShaderResources(0, 1, &renderer->m_lightSystem->m_lightsBufferSrv);
	ctx->PSSetShaderResources(1, 1, &renderer->m_lightSystem->m_shadowLightsBufferSrv);
	ctx->PSSetShaderResources(2, 1, &renderer->m_lightSystem->m_shadowDepthMapSrv);
	ctx->PSSetShaderResources(3, 1, &renderer->m_dsvShader);
	
	for (auto& io : chain->m_ioID)
	{
		ctx->RSSetViewports(io.outputCount, &VPs[io.outputIndex]);

		ctx->OMSetRenderTargets(io.outputCount, &RTVs[io.outputIndex], 0);
		ctx->PSSetShaderResources(DX11PostProcessor::START_SRV_REGISTER, io.inputCount, &SRVs[io.inputIndex]);

		ctx->PSSetShader(io.data->ps->GetNativeHandle(), 0, 0);

		if (io.data->callback)
		{
			io.data->callback(renderer, io.data->opaque);
		}

		ctx->Draw(quad->Count(), 0);

		ctx->OMSetRenderTargets(io.outputCount, nullRTVs, 0);
		ctx->PSSetShaderResources(DX11PostProcessor::START_SRV_REGISTER, io.inputCount, nullSRVs);
	}

	ctx->RSSetViewports(1, &renderer->m_viewport);

	//if not output to screen, just render last output
	if (chain->m_lastOutput != renderer->m_mainRtvShader)
	{
		ctx->OMSetRenderTargets(1, &renderer->m_mainRtv, 0);
		ctx->PSSetShaderResources(DX11PostProcessor::START_SRV_REGISTER, 1, &chain->m_lastOutput);
		ctx->PSSetShader(renderer->m_lastPresentRpl->GetPS()->GetNativeHandle(), 0, 0);
		ctx->Draw(quad->Count(), 0);
	}

	return 1;
}

DX11PostProcessor::CacheTexture2D DX11PostProcessor::AllocTexture2D(const Texture2DData& data)
{
	assert(data.tag == Texture2DData::TAG::READ_OR_WRITE);

	std::string formatDesc = "R8G8B8A8";
	int formatByteWidth = sizeof(uint32_t);

	DXGI_FORMAT format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	switch (data.type)
	{
	case Texture2DData::TYPE::R8G8B8A8:
		break;
	case Texture2DData::TYPE::R32G32B32A32:
		format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
		formatDesc = "R32G32B32A32";
		formatByteWidth = 4 * sizeof(float);
		break;
	default:
		break;
	}

	CacheTexture2D texture = {};

	ID3D11Texture2D* texture2D = 0;

	auto dev = DX11Global::renderer->m_d3dDevice;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = (UINT)data.size.x;
	desc.Height = (UINT)data.size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	if (FAILED(dev->CreateTexture2D(&desc, 0, &texture2D)))
	{
		std::cerr << "[D3D11 FAILED]: CreateTexture2D() failed\n";
		return {};
	}

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;

	if (FAILED(dev->CreateRenderTargetView(texture2D, &rtvDesc, &texture.rtv)))
	{
		std::cerr << "[D3D11 FAILED]: CreateShaderResourceView() failed\n";
		texture2D->Release();
		return {};
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

	if (FAILED(dev->CreateShaderResourceView(texture2D , &srvDesc, &texture.srv)))
	{
		std::cerr << "[D3D11 FAILED]: CreateShaderResourceView() failed\n";
		texture2D->Release();
		return {};
	}

	texture2D->Release();

	std::cout << "[DX11PostProcessor]: create new Texture2D(format = " 
		<< formatDesc 
		<< ", size = (" << (int)data.size.x << ", " << (int)data.size.y << ")"
		<< ", total bytes = " << ((int)(data.size.x * data.size.y)) * formatByteWidth << ")\n";

	return texture;
}

DX11PostProcessor::CacheTexture2D* DX11PostProcessor::TryGetTexture2D(
	std::vector<DX11PostProcessor::CacheTexture2D>& textures, size_t id, const Texture2DData& data)
{
	if (data.id >= textures.size())
	{
		textures.resize(data.id + 1);
		textures[data.id] = AllocTexture2D(data);
	}

	return &textures[data.id];
}

void DX11PostProcessor::AllocLayer(Layer& layer, DX11PostProcessChain* chain, bool input)
{
	ID3D11ShaderResourceView* output = 0;

	for (auto& texture : layer.textures)
	{
		CacheTexture2D* target = 0;
		if (texture.tag != Texture2DData::TAG::READ_OR_WRITE)
		{
			target = &m_readOnlyTextures[texture.id];
		}
		else
		{
			auto& currentId = texture.id;

			auto& availableTextures = m_textures[texture.type][texture.size];

			auto cache = TryGetTexture2D(availableTextures, currentId, texture);

			auto it = m_inUseTextures.find(*cache);

			if (it != m_inUseTextures.end())
			{
				while (it != m_inUseTextures.end())
				{
					cache = TryGetTexture2D(availableTextures, ++currentId, texture);
					it = m_inUseTextures.find(*cache);
				}

				target = cache;
			}
			else
			{
				target = cache;
			}
			
		}

		if (input)
		{
			m_inUseTextures.insert(*target);

			m_tempInputTextures.insert(*target);

			chain->m_SRVs.push_back(target->srv);
		}
		else
		{
			if (output == 0) output = target->srv;

			if (texture.tag != Texture2DData::TAG::READ_OR_WRITE)
			{
				chain->m_VPs.push_back(DX11Global::renderer->m_viewport);

				if (texture.tag == Texture2DData::TAG::SCREEN_BUFFER)
				{
					chain->m_RTVs.push_back(target->rtv);
				}
				else
				{
					chain->m_RTVs.push_back(0);
				}
			}
			else
			{
				chain->m_RTVs.push_back(target->rtv);
				D3D11_VIEWPORT vp = {};
				vp.Width = (uint32_t)texture.size.x;
				vp.Height = (uint32_t)texture.size.y;
				chain->m_VPs.push_back(vp);
			}
		}
	}

	if (!input)
	{
		for (auto& t : m_tempInputTextures)
		{
			m_inUseTextures.erase(t);
		}
		m_tempInputTextures.clear();
	}

	chain->m_lastOutput = output;
}

void DX11PostProcessor::AllocIOLayer(Layer& inputLayer, Layer& outputLayer, DX11PostProcessChain* chain)
{
	AllocLayer(inputLayer, chain, true);
	AllocLayer(outputLayer, chain, false);

	DX11PostProcessor::DX11PostProcessChain::InOutIndex io = {};

	io.inputCount = inputLayer.textures.size();
	io.outputCount = outputLayer.textures.size();

	if (chain->m_ioID.size() == 0)
	{
		io.inputIndex = 0;
		io.outputIndex = 0;		
	}
	else
	{
		auto& prev = chain->m_ioID.back();

		io.inputIndex = prev.inputIndex + prev.inputCount;
		io.outputIndex = prev.outputIndex + prev.outputCount;
	}

	chain->m_ioID.push_back(io);
}


std::string DX11PostProcessor::DX11PostProcessChain::Craft()
{
	std::string ret = "OK";

	auto& textures = m_owner->m_textures;
	m_owner->m_inUseTextures.clear();

	std::map<std::string, std::vector<InOutIndex>> alloctedProgram;

	std::vector<InOutIndex> tempAlloc;

	std::vector<std::vector<InOutIndex>> prevAlloc;
	std::vector<std::vector<InOutIndex>> curAlloc;

	Adapter* prevAdapter = 0;

	std::vector<CacheTexture2D> tempOutputs;
	std::vector<std::vector<CacheTexture2D>> prevOutputs;
	std::vector<std::vector<CacheTexture2D>> curOutputs;
	//CacheTexture2D* curOutput = 0;

	for (auto& layer : m_layers)
	{
		auto& programs = layer.m_programs;
		auto& adapter = layer.m_adapter;

		curAlloc.clear();
		curOutputs.clear();

		//alloc for each program
		for (auto& pair : programs)
		{
			tempAlloc.clear();
			tempOutputs.clear();

			auto& program = pair.first;

			// caching
			auto it = alloctedProgram.find(program->m_name);

			if (it != alloctedProgram.end())
			{
				if (pair.second)
				{
					// [TODO]
					// re-use output
					assert(0);
				}
				else
				{
					auto& cache = it->second;
					m_ioID.insert(m_ioID.end(), cache.begin(), cache.end());
				}

				continue;
			}


			// alloc new
			auto& layers = program->m_layers;
			for (size_t i = 0; i < layers.size() - 1; i++)
			{
				auto& input = layers[i];
				auto& ouput = layers[i + 1];

				m_owner->AllocIOLayer(input, ouput, this);

				// save temp alloc
				tempAlloc.push_back(m_ioID.back());

				m_ioID.back().data = &program->m_datas[i];

				//transfer to m_owner->m_ps
				auto ref = Resource::Get<PixelShader>(program->m_datas[i].ps);
				auto setRet = m_owner->m_ps.insert(ref);
				if (!setRet.second)
				{
					Resource::Release<PixelShader>(&ref);
				}
			}

			// save in-use texture
			auto& programOutput = layers.back();
			for (auto& texture : programOutput.textures)
			{
				if (texture.tag == Texture2DData::TAG::READ_OR_WRITE)
				{
					auto* _output = &textures[texture.type][texture.size][texture.id];
					m_owner->m_inUseTextures.insert(*_output);
					tempOutputs.push_back(*_output);
				}
				else
				{
					tempOutputs.push_back(m_owner->m_readOnlyTextures[texture.id]);
				}
			}

			alloctedProgram[program->m_name] = tempAlloc;

			curAlloc.push_back(tempAlloc);
			curOutputs.push_back(tempOutputs);
		}


		// process adapter
		if (prevAdapter && prevAdapter->m_programsWithOutputId.size() != 0)
		{
			for (auto& ids : prevAdapter->m_programsWithOutputId)
			{
				auto& prevProgramId = ids.first.first;
				auto& outputId = ids.first.second;
				
				auto& curProgramId = ids.second.first;
				auto& inputId = ids.second.second;

				//auto& ioIndex = prevAlloc[prevProgramId].back();
				auto& ioIndex1 = curAlloc[curProgramId].front();

				//assert(outputId < ioIndex.outputCount);

				//auto outputSRV = m_SRVs[ioIndex.outputIndex + outputId];
				//auto inputSRV = m_SRVs[ioIndex1.inputIndex + inputId];

				auto outputSRV = prevOutputs[prevProgramId][outputId].srv;

				// just replace
				m_SRVs[ioIndex1.inputIndex + inputId] = outputSRV;
			}
		}

		prevAlloc = curAlloc;
		prevOutputs = curOutputs;
		prevAdapter = &adapter;
	}

	if (m_ioID.back().outputCount != 1)
	{
		ret = "[WARNING]: number output of post processing chain should be 1";
	}

	//m_RTVs[m_ioID.back().outputIndex] = DX11Global::renderer->m_mainRtv;

	return ret;
}
