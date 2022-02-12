#include "PostProcessor.h"

void PostProcessor::AllocTexture2DDatas(Layer& layer)
{
	auto& inputCounter = m_rTexture2dCounter;
	auto& lastOutputId = m_wTexture2dCounter;

	for (size_t j = 0; j < layer.textures.size(); j++)
	{
		auto& cur = layer.textures[j];

		if (cur.tag == Texture2DData::READ_OR_WRITE && cur.id == -1)
		{
			auto& c = inputCounter[cur.type][cur.size];
			cur.id = c++;

			auto conflict = lastOutputId[cur.type][cur.size] - 1;
			while (cur.id == conflict)
			{
				cur.id = c++;
			}
		}
	}
}

std::string PostProcessor::Program::Craft()
{
	std::string ret = "OK";

	auto& inputCounter = m_owner->m_rTexture2dCounter;
	auto& lastOutputId = m_owner->m_wTexture2dCounter;

	if (m_isCrafted) goto Return;

	if (m_datas.size() != m_layers.size() - 1)
	{
		ret = "[PROGRAM FORM]: ...--->[Layer]---[PixelShader]--->[Layer]---...";
	}

	m_owner->ClearCounter(lastOutputId);

	for (size_t i = 0; i < m_layers.size() - 1; i++)
	{
		m_owner->ClearCounter(inputCounter);

		auto& input = m_layers[i];
		auto& output = m_layers[i + 1];

		m_owner->AllocTexture2DDatas(input);
		m_owner->AllocTexture2DDatas(output);

		m_owner->ClearCounter(lastOutputId);
		for (size_t j = 0; j < output.textures.size(); j++)
		{
			auto& cur = output.textures[j];
			if (cur.tag == Texture2DData::READ_OR_WRITE)
			{
				lastOutputId[cur.type][cur.size] = cur.id + 1;
			}
		}
	}


Return:
	m_isCrafted = true;
	return ret;
}

PostProcessor::PostProcessor()
{
	m_wTexture2dCounter.resize(Texture2DData::TYPE::COUNT, 
		std::map<Vec2, int64_t, bool(*)(const Vec2&, const Vec2&)>(___PostProcessor_Vec2_Compare)
	);
	m_rTexture2dCounter.resize(Texture2DData::TYPE::COUNT,
		std::map<Vec2, int64_t, bool(*)(const Vec2&, const Vec2&)>(___PostProcessor_Vec2_Compare)
	);

	m_readOnlyTextureID[AvaiableTexture2D::POSITION_AND_SPECULAR]	= 0;
	m_readOnlyTextureID[AvaiableTexture2D::NORMAL_AND_SHININESS]	= 1;
	m_readOnlyTextureID[AvaiableTexture2D::COLOR]					= 2;
	m_readOnlyTextureID[AvaiableTexture2D::METALLIC_ROUGHNESS_AO]	= 3;
	m_readOnlyTextureID[AvaiableTexture2D::LIGHTED_SCENE]			= 4;
	m_readOnlyTextureID[AvaiableTexture2D::SCREEN_BUFFER]			= 5;
}

PostProcessor::~PostProcessor()
{
	for (auto& p : m_programs)
	{
		delete p.second;
	}
	m_programs.clear();

	for (auto& p : m_procChains)
	{
		delete p.second;
	}
	m_procChains.clear();
};

PostProcessor::Texture2DData PostProcessor::GetTexture2D(const std::string& name)
{
	Texture2DData ret;
	if (name == PostProcessor::AvaiableTexture2D::SCREEN_BUFFER)
	{
		ret.tag = Texture2DData::SCREEN_BUFFER;
	}
	else
	{
		ret.tag = Texture2DData::READ_ONLY;
	}
	
	ret.id = m_readOnlyTextureID[name];
	return ret;
}

PostProcessor::Texture2DData PostProcessor::GetTexture2D(const Vec2& size, Texture2DData::TYPE type)
{
	Texture2DData ret;
	ret.tag = Texture2DData::READ_OR_WRITE;
	ret.size = size;

	//auto& cur = m_rwTexture2dCounter[type][size];

	ret.id = -1;

	return ret;
}

void PostProcessor::ClearCounter(Texture2DCounter& in)
{
	for (auto& t : in)
	{
		for (auto& tt : t)
		{
			tt.second = 0;
		}
	}
}

#ifdef DX11_RENDERER
#include "../RenderAPI/DX11/DX11PostProcessor.cpp"
#endif