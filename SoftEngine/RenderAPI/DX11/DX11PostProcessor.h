#pragma once

#include <PostProcessor.h>

#include <set>

#include <d3d11.h>

class DX11Renderer;
class PixelShader;
class VertexShader;

class DX11PostProcessor : public PostProcessor
{
public:
	static constexpr size_t START_SRV_REGISTER = 4;

	class DX11PostProcessChain : public ProcessChain
	{
	public:
		struct InOutIndex
		{
			size_t inputIndex;
			size_t inputCount;
			size_t outputIndex;
			size_t outputCount;
			Program::PrevCallData* data;
		};

	public:
		//not own
		std::vector<ID3D11ShaderResourceView*> m_SRVs;
		//not own
		std::vector<ID3D11RenderTargetView*> m_RTVs;

		std::vector<D3D11_VIEWPORT> m_VPs;

		std::vector<InOutIndex> m_ioID;

		ID3D11ShaderResourceView* m_lastOutput = 0;

		DX11PostProcessor* m_owner = 0;

	public:
		// Inherited via ProcessChain
		virtual std::string Craft() override;
	};

public:
	struct CacheTexture2D
	{
		ID3D11RenderTargetView* rtv;
		ID3D11ShaderResourceView* srv;

		friend bool operator<(const CacheTexture2D& left, const CacheTexture2D& right)
		{
			return memcmp(&left, &right, sizeof(CacheTexture2D)) < 0;
		}
	};

	//own
	std::vector<
		std::map<
			Vec2,
			std::vector<CacheTexture2D>,
			bool(*)(const Vec2&, const Vec2&)
		>
	> m_textures;

	std::set<PixelShader*> m_ps;

	std::set<CacheTexture2D> m_tempInputTextures;
	std::set<CacheTexture2D> m_inUseTextures;

	//not own
	//normal, position, roughness, ...
	std::vector<CacheTexture2D> m_readOnlyTextures;

	VertexShader* m_screenVS = 0;

public:
	DX11PostProcessor(DX11Renderer* renderer);
	~DX11PostProcessor();

protected:
	void InitReadOnlyTexture(DX11Renderer* renderer);

public:
	// Inherited via PostProcessor
	virtual ProcessChain* MakeProcessChain(const std::string& name) override;

	virtual bool Run() override;

public:
	CacheTexture2D AllocTexture2D(const Texture2DData& data);

	CacheTexture2D* TryGetTexture2D(std::vector<CacheTexture2D>& textures, size_t id, const Texture2DData& data);

	void AllocLayer(Layer& layer, DX11PostProcessChain* chain, bool input);
	void AllocIOLayer(Layer& inputLayer, Layer& outputLayer, DX11PostProcessChain* chain);

};