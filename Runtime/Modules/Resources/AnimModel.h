#pragma once

#include "Resource.h"

#include "Graphics/Graphics.h"
#include "FileSystem/FileSystem.h"

#include "MainSystem/Animation/Utils/KeyFrame.h"
#include "Scene/DeferredBuffer.h"

#include "Model3DBasic.h"

NAMESPACE_BEGIN

class AnimModel : public Model3DBasic
{
public:
	using NonWeightVertex = Model3DBasic::Vertex;
	using NonAnimMesh = Model3DBasic::Mesh;

	enum ANIM_MESH_TYPE
	{
		WEIGHT_4,
		WEIGHT_8,
		WEIGHT_16
	};

	// animation system will push data to this buffer, then rendering system will read data from this buffer to render
	struct AnimMeshRenderingBufferData
	{
		std::vector<Mat4> bones;

		// data will be refered back by AnimMesh::m_model3DIdx
		std::vector<AABox> meshesAABB;
	};

	//using AnimMeshRenderingBuffer = DeferredBuffer<AnimMeshRenderingBufferData>;

	struct AnimMeshRenderingBuffer
	{
		ID id = INVALID_ID;
		DeferredBuffer<AnimMeshRenderingBufferData> buffer;
	};

	struct AnimVertex
	{
		struct WeightVertex_4
		{
			Vec3 position;
			Vec3 tangent;
			Vec3 bitangent;
			Vec3 normal;
			Vec2 textcoord;

			unsigned short boneID1[4] = {};
			float weight1[4] = {};
		};

		struct WeightVertex_8
		{
			Vec3 position;
			Vec3 tangent;
			Vec3 bitangent;
			Vec3 normal;
			Vec2 textcoord;

			unsigned short boneID1[4] = {};
			unsigned short boneID2[4] = {};
			float weight1[4] = {};
			float weight2[4] = {};
		};

		struct WeightVertex_16
		{
			Vec3 position;
			Vec3 tangent;
			Vec3 bitangent;
			Vec3 normal;
			Vec2 textcoord;

			unsigned short boneID1[4] = {};
			unsigned short boneID2[4] = {};
			unsigned short boneID3[4] = {};
			unsigned short boneID4[4] = {};

			float weight1[4] = {};
			float weight2[4] = {};
			float weight3[4] = {};
			float weight4[4] = {};
		};

		struct BoneData4
		{
			unsigned short boneID1[4] = {};
			float weight1[4] = {};
		};

		struct BoneData8
		{
			unsigned short boneID1[4] = {};
			unsigned short boneID2[4] = {};
			float weight1[4] = {};
			float weight2[4] = {};
		};

		struct BoneData16
		{
			unsigned short boneID1[4] = {};
			unsigned short boneID2[4] = {};
			unsigned short boneID3[4] = {};
			unsigned short boneID4[4] = {};

			float weight1[4] = {};
			float weight2[4] = {};
			float weight3[4] = {};
			float weight4[4] = {};
		};

		struct PositionVertex16
		{
			Vec3 position;

			unsigned short boneID[16] = {};

			float weight[16] = {};
		};

		inline static bool TryFill(void* vertex, unsigned short boneID, float weight, size_t maxWeightPerVertex)
		{
			//WeightVertex_4* p4 = (WeightVertex_4*)vertex;
			//WeightVertex_8* p8 = (WeightVertex_8*)vertex;
			//WeightVertex_16* p16 = (WeightVertex_16*)vertex;

			//const auto skip1 = sizeof(Vec3) * 4 + sizeof(Vec2);
			//const auto skip2 = skip1 + maxWeightPerVertex * sizeof(unsigned short);
			const auto skip1 = 0;
			const auto skip2 = skip1 + maxWeightPerVertex * sizeof(unsigned short);
			unsigned short* _boneID = (unsigned short*)(((uint8_t*)vertex) + skip1);
			float* _weight = (float*)(((uint8_t*)vertex) + skip2);

			for (size_t i = 0; i < maxWeightPerVertex; i++)
			{
				if (_weight[i] == 0)
				{
					_boneID[i] = boneID;
					_weight[i] = weight;
					return true;
				}
			}

			//::memset(_boneID, 0, maxWeightPerVertex * sizeof(unsigned short));
			//::memset(_weight, 0, maxWeightPerVertex * sizeof(float));

			//Throw(L"Weight per vertex too big");
			return false;
		}

	};

	class AnimMesh
	{
	public:
		SharedPtr<GraphicsVertexBuffer> m_vertexBuffer;
		uint32_t m_vertexCount;
		uint32_t m_model3DIdx;

		ANIM_MESH_TYPE m_type;

		inline const auto& GetVertexBuffer() const
		{
			return m_vertexBuffer;
		}

		inline auto GetVertexCount() const
		{
			return m_vertexCount;
		}
	};

	std::vector<AnimMesh> m_animMeshes;

	// name - id
	std::map<String, ID> m_boneIds;
	// id - name
	std::vector<String> m_boneNames;

	// list of model animation
	std::vector<Animation> m_animations;

	// inversed of node's global transform
	std::vector<Mat4> m_boneOffsetMatrixs;

	AnimModel(String path, bool placeholder = false);

};

NAMESPACE_END