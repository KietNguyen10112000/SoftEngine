#pragma once

#include "Resource.h"

#include "Graphics/Graphics.h"
#include "FileSystem/FileSystem.h"

#include "MainSystem/Animation/Utils/KeyFrame.h"

#include "Model3DBasic.h"

NAMESPACE_BEGIN

class AnimModel : public Model3DBasic
{
public:
	using NonWeightVertex = Model3DBasic::Vertex;
	using NonAnimMesh = Model3DBasic::Mesh;

	enum ANIM_MESH_TYPE
	{
		WEIGHT_4 = 4,
		WEIGHT_8 = 8,
		WEIGHT_16 = 16
	};

	struct AnimVertex
	{
		struct WeightVertex_4
		{
			Vec3 pos;
			Vec2 texcoord;
			Vec3 normal;
			Vec3 tangent;
			Vec3 bitangent;

			unsigned short boneID1[4] = {};
			float weight1[4] = {};
		};

		struct WeightVertex_8
		{
			Vec3 pos;
			Vec2 texcoord;
			Vec3 normal;
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
			Vec3 normal;
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

			const auto skip1 = sizeof(Vec3) * 4 + sizeof(Vec2);
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

	class AnimMesh : public Mesh
	{
	public:
		SharedPtr<GraphicsVertexBuffer> m_vertexBuffer;
		uint32_t m_vertexCount;
		uint32_t m_model3DIdx;
		AABox m_aabb;

		ANIM_MESH_TYPE m_type;

		inline const auto& GetVertexBuffer() const
		{
			return m_vertexBuffer;
		}

		inline auto GetVertexCount() const
		{
			return m_vertexCount;
		}

		inline AABox GetLocalAABB() const
		{
			return m_aabb;
		}
	};

	std::vector<AnimMesh> m_animMeshes;

	AnimModel(String path, bool placeholder = false);

};

NAMESPACE_END