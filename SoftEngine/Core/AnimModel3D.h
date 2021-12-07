#pragma once

#include <IResource.h>

#include <Math/Math.h>
#include <Buffer.h>

#include <vector>

#define MAX_BONE 512

class RenderPipeline;
class IRenderer;

struct _TBNRenderBuffer
{
	VertexBuffer* vb = 0;
	IndexBuffer* ib = 0;

	Mat4x4 localTransform;

	//refer to m_rpls
	int rplIndex = 0;
};

template<typename T> class AnimModel;
typedef AnimModel<_TBNRenderBuffer> TBNAnimModel;
//class TBNAnimModel;

class TBNAnimModelLoader
{
public:
	struct AnimMesh
	{
		enum MAX_WEIGHT_PER_VERTEX
		{
			FOUR	= 4,
			EIGHT	= 8,
			SIXTEEN = 16
		};

		struct WeightVertex_4
		{
			Vec3 pos;
			Vec2 texcoord;
			Vec3 tangent;
			Vec3 bitangent;

			unsigned short boneID1[4] = {};
			float weight1[4] = {};
		};

		struct WeightVertex_8
		{
			Vec3 pos;
			Vec2 texcoord;
			Vec3 tangent;
			Vec3 bitangent;

			unsigned short boneID1[4] = {};
			unsigned short boneID2[4] = {};
			float weight1[4] = {};
			float weight2[4] = {};
		};

		struct WeightVertex_16
		{
			Vec3 pos;
			Vec2 texcoord;
			Vec3 tangent;
			Vec3 bitangent;

			unsigned short boneID1[4] = {};
			unsigned short boneID2[4] = {};
			unsigned short boneID3[4] = {};
			unsigned short boneID4[4] = {};

			float weight1[4] = {};
			float weight2[4] = {};
			float weight3[4] = {};
			float weight4[4] = {};
		};

		inline static void TryFill(void* vertex, int boneID, float weight, size_t maxWeightPerVertex)
		{
			//WeightVertex_4* p4 = (WeightVertex_4*)vertex;
			//WeightVertex_8* p8 = (WeightVertex_8*)vertex;
			//WeightVertex_16* p16 = (WeightVertex_16*)vertex;

			const auto skip1 = sizeof(Vec3) * 3 + sizeof(Vec2);
			const auto skip2 = skip1 + maxWeightPerVertex * sizeof(unsigned short);
			unsigned short* _boneID = (unsigned short*)(((uint8_t*)vertex) + skip1);
			float* _weight = (float*)(((uint8_t*)vertex) + skip2);

			for (size_t i = 0; i < maxWeightPerVertex; i++)
			{
				if (_weight[i] == 0)
				{
					_boneID[i] = boneID;
					_weight[i] = weight;
					return;
				}
			}

			//::memset(_boneID, 0, maxWeightPerVertex * sizeof(unsigned short));
			//::memset(_weight, 0, maxWeightPerVertex * sizeof(float));

			Throw(L"Weight per vertex too big");
		}

	};

	struct NonAnimMesh
	{
		struct Vertex
		{
			Vec3 pos;
			Vec2 texcoord;
			Vec3 tangent;
			Vec3 bitangent;
		};
	};

public:
	static void Load(const std::wstring& path, Mat4x4& preTransform, TBNAnimModel* init);

public:
	static RenderPipeline* GetNonAnimMeshRpl();
	static RenderPipeline* GetAnimMeshRpl(size_t maxWeightPerVertex);

};

template<class T>
struct KeyFrame
{
	T value = {};
	float time = 0;
};

typedef KeyFrame<Vec3> ScalingKeyFrame;
typedef KeyFrame<Vec4> RotaionKeyFrame;
typedef KeyFrame<Vec3> TranslationKeyFrame;

struct KeyFrames
{
	union
	{
		//int boneId;
		//or
		int nodeId;
	};
	std::vector<ScalingKeyFrame> scaling;
	std::vector<RotaionKeyFrame> rotation;
	std::vector<TranslationKeyFrame> translation;
};

struct Animation
{
	int id = -1;

	std::string name;
	float tickDuration = 0;
	float ticksPerSecond = 0;

	std::vector<KeyFrames> channels;
};

struct _AnimModelNode
{
	//std::string name;
	int parentIndex;
	int boneIndex = -1;

	uint32_t numMeshs;
	uint32_t* meshsIndex; //refer to m_renderBuf[i]

	Mat4x4 localTransform;

	inline ~_AnimModelNode() { delete meshsIndex; };
};

//args[0] = preTransform
//args[1] = AnimModel::TYPE
template<typename _RenderBuffer>
class AnimModel : public IResource
{
public:
	enum TYPE
	{
		MaterialAnimModel = 0,
		TBNAnimModel = 1
	};

public:
	inline static ShaderVar* localSV = 0;
	inline static DynamicShaderVar* boneSV = 0;

public:
	//[0] : non anim mesh rpl
	//[1] : 4 weighted anim mesh rpl
	//[2] : 8 weighted anim mesh rpl
	//[3] : 16 weighted anim mesh rpl
	RenderPipeline* m_rpls[4] = { 0,0 };
	std::vector<_RenderBuffer> m_renderBuf;

	std::vector<Animation> m_animations;

	//that transform from mesh space to bone space
	std::vector<Mat4x4> m_bonesOffset;
	//std::vector<int> m_bonesParentId;

	std::vector<_AnimModelNode> m_nodes;

public:
	AnimModel(const std::wstring& path, uint32_t numArg, void** args);
	~AnimModel();

public:
	inline static void FreeStaticResource()
	{
		delete localSV;
		localSV = 0;
		delete boneSV;
		boneSV = 0;
	}

public:
	void Render(IRenderer* renderer);

	void Render(IRenderer* renderer, std::vector<Mat4x4>& meshLocalTransform, std::vector<Mat4x4>& bones);

	void Render(IRenderer* renderer, std::vector<Mat4x4*>& meshLocalTransform, std::vector<Mat4x4>& bones);
};

template<typename _RenderBuffer>
inline AnimModel<_RenderBuffer>::AnimModel(const std::wstring& path, uint32_t numArg, void** args) : IResource(path)
{
	//assert(numArg == 2);
	//size_t type = (size_t)args[1];
	Mat4x4* preTransform = (Mat4x4*)args[0];

	if (localSV == 0)
	{
		localSV = new ShaderVar(preTransform, sizeof(Mat4x4));
		std::vector<Mat4x4> temp = std::vector<Mat4x4>(MAX_BONE);
		boneSV = new DynamicShaderVar(&temp[0], sizeof(Mat4x4) * temp.size());
	}

	if (std::is_same_v<_RenderBuffer, _TBNRenderBuffer>)
	{
		TBNAnimModelLoader::Load(path, *preTransform, this);
	}
	else
	{
		assert(0);
	}

	for (size_t i = 0; i < m_animations.size(); i++)
	{
		m_animations[i].id = i;
	}
}

template<typename _RenderBuffer>
inline AnimModel<_RenderBuffer>::~AnimModel()
{
	for (auto& buf : m_renderBuf)
	{
		delete buf.ib;
		delete buf.vb;
	}
	m_nodes.clear();

	for (size_t i = 0; i < 4; i++)
	{
		if (m_rpls[i])
		{
			RenderPipelineManager::Release(&m_rpls[i]);
		}
	}
}

template<typename _RenderBuffer>
inline void AnimModel<_RenderBuffer>::Render(IRenderer* renderer)
{
	RenderPipeline::VSSetVar(localSV, 2);
	RenderPipeline::VSSetVar(boneSV, 3);
	//boneSV->Update(&m_bonesOffset, sizeof(Mat4x4) * m_bonesOffset.size());
	for (auto& buf : m_renderBuf)
	{
		localSV->Update(&buf.localTransform, sizeof(Mat4x4));
		renderer->Render(m_rpls[buf.rplIndex], buf.vb, buf.ib);
	}
}

template<typename _RenderBuffer>
inline void AnimModel<_RenderBuffer>::Render(IRenderer* renderer, 
	std::vector<Mat4x4>& meshLocalTransform, std::vector<Mat4x4>& bones)
{
	RenderPipeline::VSSetVar(localSV, 2);
	RenderPipeline::VSSetVar(boneSV, 3);
	boneSV->Update(&bones[0], sizeof(Mat4x4) * bones.size());

	size_t i = 0;
	for (auto& buf : m_renderBuf)
	{
		localSV->Update(&meshLocalTransform[i], sizeof(Mat4x4));
		renderer->Render(m_rpls[buf.rplIndex], buf.vb, buf.ib);
		i++;
	}
}

template<typename _RenderBuffer>
inline void AnimModel<_RenderBuffer>::Render(IRenderer* renderer,
	std::vector<Mat4x4*>& meshLocalTransform, std::vector<Mat4x4>& bones)
{
	RenderPipeline::VSSetVar(localSV, 2);
	RenderPipeline::VSSetVar(boneSV, 3);
	boneSV->Update(&bones[0], sizeof(Mat4x4) * bones.size());

	size_t i = 0;
	for (auto& buf : m_renderBuf)
	{
		if(meshLocalTransform[i]) localSV->Update(meshLocalTransform[i], sizeof(Mat4x4));
		renderer->Render(m_rpls[buf.rplIndex], buf.vb, buf.ib);
		i++;
	}
}