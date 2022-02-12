#pragma once
#include <vector>
#include <string>

#include <map>

#include <Math/Math.h>

class PixelShader;
class IRenderer;

inline bool ___PostProcessor_Vec2_Compare(const Vec2& l, const Vec2& r)
{
	return memcmp(&l, &r, sizeof(Vec2)) > 0;
}

class PostProcessor
{
public:
	struct AvaiableTexture2D
	{
		//float4(pos.x, pos.y, pos.z, spec)
		constexpr static const char* POSITION_AND_SPECULAR = "Pos+Spec";
		//float4(nor.x, nor.y, nor.z, shininess)
		constexpr static const char* NORMAL_AND_SHININESS = "Nor+Shi";
		//float4
		constexpr static const char* COLOR = "Color";
		//float4(metallic, roughness, ao, non-use-yet)
		constexpr static const char* METALLIC_ROUGHNESS_AO = "M+R+O";

		//the scene with direct lighting
		//r8g8b8a8_unorm
		constexpr static const char* LIGHTED_SCENE = "LightedScene";
		//24 bit depth + 8 bit stencil
		//alway at register(t3)
		constexpr static const char* SCENE_DEPTH = "Depth";

		constexpr static const char* SCREEN_BUFFER = "ScreenBuffer";
	};

	struct Texture2DData
	{
		enum TAG
		{
			READ_OR_WRITE,
			READ_ONLY,
			SCREEN_BUFFER
		};

		enum TYPE
		{
			R8G8B8A8,
			R32G32B32A32,
			COUNT
		};

		TAG tag;
		TYPE type = R8G8B8A8;

		Vec2 size;
		long long id;

		inline bool operator==(const Texture2DData& left)
		{
			return memcmp(&left, this, sizeof(Texture2DData)) == 0;
		}
	};

	struct Layer
	{
		static constexpr size_t MAX_TEXTURE = 8;
		std::vector<Texture2DData> textures;
	};

	class Program
	{
	public:
		struct PrevCallData
		{
			PixelShader* ps;
			void* opaque = 0;
			void(*callback)(IRenderer*, void*) = 0;
		};
		std::vector<Layer> m_layers;
		std::vector<PrevCallData> m_datas;

		bool m_isCrafted = false;
		PostProcessor* m_owner = 0;

		std::string m_name;

	public:
		inline void Append(const Layer& layer) { m_layers.push_back(layer); };
		inline void Append(PixelShader* ps, void* opaque = 0, void(*prevCall)(IRenderer*, void*) = 0) 
		{ 
			m_datas.push_back({ ps, opaque, prevCall });
		};

		inline bool IsCrafted() { return m_isCrafted; };

		std::string Craft();
	};

	class Adapter
	{
	public:
		// <<program id, output id>, <program id, input id>>
		//([0, 0], [0, 0]) => output[0] of program[0] will be input[0] of program[0] of next layer
		std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> m_programsWithOutputId;

	public:
		inline Adapter(const std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>>& param)
			: m_programsWithOutputId(param) {};
		inline ~Adapter() {};

	};

	// output of each Program will not re-use in next programs
	// eg Program1 output will not re-use in Program2 but can use as input of Program2 via Adapter
	// Adapter can discard output of previous Program 
	// [Program0]--->|Adapter0|--->[Program2]--->|		  |
	// [Program1]--->|        |    [Program3]--->|Adapter1|---> ...
	//							   [Program4]--->|		  |
	//							 
	//						      
	//
	class ProcessChainLayer
	{
	public:
		// reRun = std::pair.secound
		// (reUse == true) => m_programs[i] will be re-run (not use prev result)
		std::vector<std::pair<Program*, bool>> m_programs;
		Adapter m_adapter;

	public:
		inline ProcessChainLayer(const std::vector<std::pair<Program*, bool>>& programs, const Adapter& adapter)
			: m_programs(programs), m_adapter(adapter) {};
	};

	//for rendering api implementation
	class ProcessChain
	{
	public:
		std::vector<ProcessChainLayer> m_layers;

		bool m_isCrafted = false;

	public:
		inline virtual ~ProcessChain() {};

	public:
		virtual std::string Craft() = 0;

		
		inline void Append(const ProcessChainLayer& layer)
		{ 
			m_layers.push_back(layer);
		};

		inline bool IsCrafted() { return m_isCrafted; };
	};

public:
	std::map<std::string, size_t> m_readOnlyTextureID;

	//READ_WRITE
	//vector[texture type]
	//map<texture size, id count>, id count reset after MakeProcessChain()
	using Texture2DCounter = std::vector<std::map<Vec2, int64_t, bool(*)(const Vec2&, const Vec2&)>>;
	Texture2DCounter m_wTexture2dCounter;
	Texture2DCounter m_rTexture2dCounter;

	std::map<std::string, Program*> m_programs;

	std::map<std::string, ProcessChain*> m_procChains;

	ProcessChain* m_activeProcChain = 0;

public:
	PostProcessor();
	virtual ~PostProcessor();

public:
	inline Layer MakeLayer(const std::vector<Texture2DData>& in);
	inline Program* MakeProgram(const std::string& name);
	

	Texture2DData GetTexture2D(const std::string& name);
	Texture2DData GetTexture2D(const Vec2& size, Texture2DData::TYPE type = Texture2DData::TYPE::R8G8B8A8);

protected:
	void ClearCounter(Texture2DCounter& in);

	void AllocTexture2DDatas(Layer& layer);

public:
	inline void SetProcessChain(ProcessChain* chain) { m_activeProcChain = chain; };

public:
	virtual ProcessChain* MakeProcessChain(const std::string& name) = 0;

	virtual bool Run() = 0;
	
};


inline PostProcessor::Layer PostProcessor::MakeLayer(const std::vector<Texture2DData>& in)
{
	Layer out;
	out.textures = in;
	out.textures.resize(
		in.size() >= PostProcessor::Layer::MAX_TEXTURE ? PostProcessor::Layer::MAX_TEXTURE : in.size()
	);
	return out;
}

inline PostProcessor::Program* PostProcessor::MakeProgram(const std::string& name)
{
	auto& ret = m_programs[name];

	if (ret == 0)
	{
		ret = new Program();
		ret->m_name = name;
		ret->m_owner = this;
	}

	return ret;
}
