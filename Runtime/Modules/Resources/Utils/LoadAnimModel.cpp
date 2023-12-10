#include "Utils.h"

#include "../AnimModel.h"

#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"
#include "MainSystem/Rendering/Components/AnimModelStaticMeshRenderer.h"
#include "MainSystem/Rendering/Components/AnimMeshRenderer.h"
#include "MainSystem/Animation/Components/AnimSkeletalGameObject.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalGameObject.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"

#include "Runtime/Runtime.h"
#include "Scene/GameObjectCache.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

NAMESPACE_BEGIN

namespace ResourceUtils
{
extern void LoadAllMeshsForModel3DBasic(Model3DBasic* model3D, const aiScene* scene, bool ignoreBones);

Mat4 ConvertAssimpMat4(const aiMatrix4x4& from)
{
	Mat4 to;
	/*for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			ret[i][j] = mat[i][j];
		}
	}*/
	to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
	to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
	to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
	to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
	return to;
}

struct AnimModelLoadingCtx
{
	struct Node
	{
		ID id = INVALID_ID;
		ID parentId = INVALID_ID;
		ID boneId = INVALID_ID;
		Mat4 localTransform;
		//Mat4 globalTransform;
	};

	struct AnimMeshVertices
	{
		std::vector<AnimModel::AnimVertex::PositionVertex16> vertices;
		std::vector<uint32_t> indices;
	};

	struct CalAABBTaskCtx
	{
		std::vector<Mat4> globalTransform;
		std::vector<Mat4> bonesMat;
		std::vector<KeyFramesIndex> index;
	};

	struct CalAABBTaskParam1
	{
		AnimModelLoadingCtx* ctx;
		Animation* animation;
		AnimModel::AnimMesh* mesh;
		Resource<AnimModel> model;
		const aiScene* scene;
	};

	struct CalAABBTaskParam2
	{
		CalAABBTaskParam1* param1;
		AABoxKeyFrame* output;
	};

	// refer to m_animMeshes/m_meshes
	std::vector<void*> meshes;

	SharedPtr<AnimModel::AnimMeshRenderingBuffer> animMeshRenderingBuffer;
	AnimatorSkeletalGameObject* animator;
	AnimatorSkeletalArray* animatorArray;

	std::vector<Node> nodes;

	// refered by AnimModel::AnimMesh::m_model3DIdx
	std::vector<AnimMeshVertices> animMeshesVertices;

	//std::vector<KeyFramesIndex> keyFramesIndex;
	//std::vector<Mat4> tempBones;

	std::map<String, GameObject*> objectMap;

	std::vector<CalAABBTaskCtx> calAABBTaskCtxs;

	inline AnimModelLoadingCtx(AnimModel* model, const aiScene* scene)
	{
		auto& nonAnimMeshes = model->m_meshes;
		auto& animMeshes = model->m_animMeshes;
		uint32_t nonAnimMeshCount = 0;
		uint32_t animMeshCount = 0;
		meshes.resize(scene->mNumMeshes);

		for (uint32_t i = 0; i < scene->mNumMeshes; i++)
		{
			auto aiMesh = scene->mMeshes[i];

			if (!aiMesh->HasBones())
			{
				meshes[i] = &nonAnimMeshes[nonAnimMeshCount];
				nonAnimMeshCount++;
				continue;
			}

			meshes[i] = &animMeshes[animMeshCount];
			animMeshCount++;
		}

		AnimModel::AnimMeshRenderingBufferData buffer;
		buffer.bones.resize(model->m_boneIds.size());
		buffer.meshesAABB.resize(model->m_animMeshes.size());
		animMeshRenderingBuffer = std::make_shared<AnimModel::AnimMeshRenderingBuffer>();
		animMeshRenderingBuffer->buffer.Initialize(buffer);

		animMeshesVertices.resize(model->m_animMeshes.size());
	}
};

void LoadAllAnimMeshsForAnimModel(AnimModel* model, const aiScene* scene)
{
	constexpr static auto LoadAnimMesh = [](AnimModel* model, AnimModel::AnimMesh* output, const aiScene* scene, aiMesh* mesh) -> void
	{
		auto graphics = Graphics::Get();

		auto numVertices = mesh->mNumVertices;
		auto aiVertices = mesh->mVertices;

		std::vector<byte> verticesBoneData;
		verticesBoneData.resize(numVertices * sizeof(AnimModel::AnimVertex::BoneData16), 0);

		size_t totalVertices = mesh->mNumFaces * mesh->mFaces[0].mNumIndices;
		std::vector<byte> verticesBuffer;
		verticesBuffer.resize(totalVertices * sizeof(AnimModel::AnimVertex::WeightVertex_16), 0);

		auto textCoord = mesh->mTextureCoords[0];

		auto verticesBoneDataIt = verticesBoneData.data();
		size_t vertexTypeIdx = 0;

		constexpr size_t vertexBoneTypeSizes[] = {
			sizeof(AnimModel::AnimVertex::BoneData4),
			sizeof(AnimModel::AnimVertex::BoneData8),
			sizeof(AnimModel::AnimVertex::BoneData16)
		};

		constexpr size_t maxWeightPerVertex[] = {
			4,
			8,
			16
		};

		constexpr size_t NUM_TYPE = (sizeof(vertexBoneTypeSizes) / sizeof(vertexBoneTypeSizes[0]));

		size_t curVertexBoneTypeSize = vertexBoneTypeSizes[vertexTypeIdx];
		size_t curMaxWeightPerVertex = maxWeightPerVertex[vertexTypeIdx];

		while (vertexTypeIdx < NUM_TYPE)
		{
			for (uint32_t i = 0; i < mesh->mNumBones; i++)
			{
				auto bone = mesh->mBones[i];
				String name = bone->mName.C_Str();

				assert(model->m_boneIds.find(name) != model->m_boneIds.end());

				auto boneId = model->m_boneIds[name];

				auto weights = bone->mWeights;
				auto numWeights = bone->mNumWeights;

				for (uint32_t weightIndex = 0; weightIndex < numWeights; ++weightIndex)
				{
					auto vertexId = weights[weightIndex].mVertexId;
					float weight = weights[weightIndex].mWeight;

					if (!AnimModel::AnimVertex::TryFill(verticesBoneDataIt + curVertexBoneTypeSize * vertexId, (uint16_t)boneId, weight, curMaxWeightPerVertex))
					{
						goto Next;
					}
				}
			}

			break;
		Next:
			vertexTypeIdx++;
			curVertexBoneTypeSize = vertexBoneTypeSizes[vertexTypeIdx];
			curMaxWeightPerVertex = maxWeightPerVertex[vertexTypeIdx];
			verticesBoneDataIt = verticesBoneData.data();
			::memset(verticesBoneDataIt, 0, verticesBoneData.size() * sizeof(byte));
		}

		if (vertexTypeIdx == NUM_TYPE)
		{
			assert(0 && "Too many bones influence to 1 vertex");
		}

		constexpr size_t vertexTypeSizes[] = {
			sizeof(AnimModel::AnimVertex::WeightVertex_4),
			sizeof(AnimModel::AnimVertex::WeightVertex_8),
			sizeof(AnimModel::AnimVertex::WeightVertex_16)
		};

		auto vertexTypeSize = vertexTypeSizes[vertexTypeIdx];

		size_t _idx = 0;
		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace& face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; j++)
			{
				auto idx = face.mIndices[j];
				auto& aiVertex = aiVertices[idx];

				auto& vertex = *(AnimModel::AnimVertex::WeightVertex_16*)(verticesBuffer.data() + _idx * vertexTypeSize);

				vertex.position = Vec3(aiVertex.x, aiVertex.y, aiVertex.z);

				if (textCoord) // does the mesh contain texture coordinates?
				{
					vertex.textcoord.x = textCoord[idx].x;
					vertex.textcoord.y = textCoord[idx].y;
				}

				if (mesh->mTangents)
				{
					vertex.tangent.x = mesh->mTangents[idx].x;
					vertex.tangent.y = mesh->mTangents[idx].y;
					vertex.tangent.z = mesh->mTangents[idx].z;
				}

				if (mesh->mBitangents)
				{
					vertex.bitangent.x = mesh->mBitangents[idx].x;
					vertex.bitangent.y = mesh->mBitangents[idx].y;
					vertex.bitangent.z = mesh->mBitangents[idx].z;
				}

				if (mesh->mNormals)
				{
					vertex.normal.x = mesh->mNormals[idx].x;
					vertex.normal.y = mesh->mNormals[idx].y;
					vertex.normal.z = mesh->mNormals[idx].z;
				}

				auto* srcBoneDataIt = (AnimModel::AnimVertex::BoneData16*)(verticesBoneData.data() + idx * curVertexBoneTypeSize);
				auto boneDataIt = (byte*)&vertex.boneID1[0];
				::memcpy(boneDataIt, srcBoneDataIt, curVertexBoneTypeSize);

				_idx++;
			}
		}

		GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC vbDesc = {};
		vbDesc.count = totalVertices;
		vbDesc.stride = vertexTypeSize;
		output->m_vertexBuffer = graphics->CreateVertexBuffer(vbDesc);
		output->m_vertexBuffer->UpdateBuffer(verticesBuffer.data(), vbDesc.count * vbDesc.stride);

		output->m_vertexCount = vbDesc.count;

		output->m_type = (AnimModel::ANIM_MESH_TYPE)vertexTypeIdx;
	};

	constexpr static auto IndexingAllBones = [](AnimModel* model, const aiScene* scene) -> void
	{
		auto& map = model->m_boneIds;
		for (uint32_t i = 0; i < scene->mNumMeshes; i++)
		{
			auto aiMesh = scene->mMeshes[i];
			if (!aiMesh->HasBones())
			{
				continue;
			}

			uint32_t numBones = aiMesh->mNumBones;
			auto bones = aiMesh->mBones;
			for (uint32_t j = 0; j < numBones; j++)
			{
				auto bone = bones[j];
				String name = bone->mName.C_Str();
				if (map.find(name) != map.end())
				{
					continue;
				}

				map.insert({ name, map.size() });
				model->m_boneNames.push_back(name);
				model->m_boneOffsetMatrixs.push_back(ConvertAssimpMat4(bone->mOffsetMatrix));
			}
		}
	};

	IndexingAllBones(model, scene);

	auto numAnimMeshes = model->m_boneIds.size();
	model->m_animMeshes.resize(numAnimMeshes);
	uint32_t count = 0;
	for (uint32_t i = 0; i < scene->mNumMeshes; i++)
	{
		auto aiMesh = scene->mMeshes[i];
		if (!aiMesh->HasBones())
		{
			continue;
		}

		auto mesh = &model->m_animMeshes[count];

		LoadAnimMesh(model, mesh, scene, aiMesh);

		mesh->m_model3DIdx = count;

		count++;
	}

	model->m_animMeshes.resize(count);
}

void LoadAnimModelAnimation(AnimModelLoadingCtx* ctx, Resource<AnimModel>& model, const aiScene* scene)
{
	constexpr static auto ExtractScaling = [](aiNodeAnim* aiNode, KeyFrames& keyFrames)
	{
		auto num = aiNode->mNumScalingKeys;
		auto aiScalings = aiNode->mScalingKeys;

		keyFrames.scaling.resize(num);

		for (uint32_t i = 0; i < num; i++)
		{
			auto& aiScaling = aiScalings[i];
			auto& scaling = keyFrames.scaling[i];

			scaling.time = aiScaling.mTime;
			scaling.value.x = aiScaling.mValue.x;
			scaling.value.y = aiScaling.mValue.y;
			scaling.value.z = aiScaling.mValue.z;
		}
	};

	constexpr static auto ExtractRotation = [](aiNodeAnim* aiNode, KeyFrames& keyFrames)
	{
		auto num = aiNode->mNumRotationKeys;
		auto aiKeys = aiNode->mRotationKeys;

		keyFrames.rotation.resize(num);

		for (uint32_t i = 0; i < num; i++)
		{
			auto& aiKey = aiKeys[i];
			auto& key = keyFrames.rotation[i];

			key.time = aiKey.mTime;
			key.value.x = aiKey.mValue.x;
			key.value.y = aiKey.mValue.y;
			key.value.z = aiKey.mValue.z;
			key.value.w = aiKey.mValue.w;
		}
	};

	constexpr static auto ExtractTranslation = [](aiNodeAnim* aiNode, KeyFrames& keyFrames)
	{
		auto num = aiNode->mNumPositionKeys;
		auto aiKeys = aiNode->mPositionKeys;

		keyFrames.translation.resize(num);

		for (uint32_t i = 0; i < num; i++)
		{
			auto& aiKey = aiKeys[i];
			auto& key = keyFrames.translation[i];

			key.time = aiKey.mTime;
			key.value.x = aiKey.mValue.x;
			key.value.y = aiKey.mValue.y;
			key.value.z = aiKey.mValue.z;
		}
	};

	auto numAnimations = scene->mNumAnimations;

	auto& animations = model->m_animations;
	animations.resize(numAnimations);

	auto numBones = model->m_boneIds.size();

	auto& boneIds = model->m_boneIds;

	for (uint32_t i = 0; i < numAnimations; i++)
	{
		auto& animation = animations[i];
		animation.channels.resize(numBones);
		animation.animMeshLocalAABoxKeyFrames.resize(model->m_animMeshes.size());
	}

	for (uint32_t i = 0; i < numAnimations; i++)
	{
		auto aiAnim = scene->mAnimations[i];
		auto& animation = animations[i];

		animation.name = aiAnim->mName.C_Str();
		animation.tickDuration = aiAnim->mDuration;
		animation.ticksPerSecond = aiAnim->mTicksPerSecond;

		auto numChannels = aiAnim->mNumChannels;
		for (uint32_t j = 0; j < numChannels; j++)
		{
			auto aiAnimNode = aiAnim->mChannels[j];
			String affectedNodeName = aiAnimNode->mNodeName.C_Str();

			//assert(boneIds.find(affectedNodeName) != boneIds.end());

			if (boneIds.find(affectedNodeName) == boneIds.end())
			{
				for (auto& a : animations)
				{
					assert(a.channels.size() == boneIds.size());
					a.channels.emplace_back();
				}

				auto& buf = ctx->animMeshRenderingBuffer->buffer.Buffers();
				for (auto& b : buf)
				{
					b.bones.emplace_back();
				}

				auto boneId = boneIds.size();
				boneIds.insert({ affectedNodeName, boneId });

				model->m_boneOffsetMatrixs.emplace_back();

				//assert(ctx->objectMap.find(affectedNodeName) != ctx->objectMap.end());

				if (ctx->objectMap.find(affectedNodeName) != ctx->objectMap.end())
				{
					auto obj = ctx->objectMap[affectedNodeName];
					if (obj->GetComponentRaw<AnimSkeletalGameObject>() == nullptr)
					{
						auto comp = obj->NewComponent<AnimSkeletalGameObject>();
						comp->m_model3D = model;
						comp->m_boneId = boneId;
						comp->m_animMeshRenderingBuffer = ctx->animMeshRenderingBuffer;
						comp->m_animator = ctx->animator;
					}
				}
				
			}

			auto affectedBoneId = boneIds[affectedNodeName];
			auto& channel = animation.channels[affectedBoneId];
			
			ExtractScaling(aiAnimNode, channel);
			ExtractRotation(aiAnimNode, channel);
			ExtractTranslation(aiAnimNode, channel);
		}
	}
}

void LoadAnimMeshVertices(AnimModelLoadingCtx* ctx, AnimModelLoadingCtx::AnimMeshVertices* animMeshVertices, AnimModel* model, aiMesh* mesh)
{
	auto& vertices = animMeshVertices->vertices;
	auto numVertices = mesh->mNumVertices;
	vertices.resize(numVertices);

	for (uint32_t i = 0; i < numVertices; i++)
	{
		auto& aiVertex = mesh->mVertices[i];
		auto& vertex = vertices[i];
		vertex.position.x = aiVertex.x;
		vertex.position.y = aiVertex.y;
		vertex.position.z = aiVertex.z;
	}

	for (uint32_t i = 0; i < mesh->mNumBones; i++)
	{
		auto bone = mesh->mBones[i];
		String name = bone->mName.C_Str();

		assert(model->m_boneIds.find(name) != model->m_boneIds.end());

		auto boneId = model->m_boneIds[name];

		auto weights = bone->mWeights;
		auto numWeights = bone->mNumWeights;

		for (uint32_t weightIndex = 0; weightIndex < numWeights; ++weightIndex)
		{
			auto vertexId = weights[weightIndex].mVertexId;
			float weight = weights[weightIndex].mWeight;

			if (!AnimModel::AnimVertex::TryFill(&(vertices[vertexId].boneID[0]), (uint16_t)boneId, weight, 16))
			{
				assert(0);
			}
		}
	}

	for (uint32_t i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace& face = mesh->mFaces[i];
		for (uint32_t j = 0; j < face.mNumIndices; j++)
		{
			animMeshVertices->indices.push_back(face.mIndices[j]);
		}
	}
}

void FlattenAnimModelHierarchy(AnimModelLoadingCtx* ctx, Resource<AnimModel>& model, const aiScene* scene)
{
	constexpr static void (*ProcessNode)(AnimModelLoadingCtx*, Resource<AnimModel>&, const aiScene*, aiNode*, ID) =
		[](AnimModelLoadingCtx* ctx, Resource<AnimModel>& model, const aiScene* scene, aiNode* aiNode, ID parentId) -> void
	{
		for (size_t i = 0; i < aiNode->mNumMeshes; i++)
		{
			auto aiMesh = scene->mMeshes[aiNode->mMeshes[i]];
			if (!aiMesh->HasBones())
			{
				continue;
			}

			auto animMeshId = ((AnimModel::AnimMesh*)ctx->meshes[aiNode->mMeshes[i]])->m_model3DIdx;
			LoadAnimMeshVertices(ctx, &ctx->animMeshesVertices[animMeshId], model, aiMesh);
		}

		AnimModelLoadingCtx::Node node = {};
		node.id = ctx->nodes.size();
		node.parentId = parentId;
		node.localTransform = ConvertAssimpMat4(aiNode->mTransformation);
		
		String maybeBoneName = aiNode->mName.C_Str();
		if (!maybeBoneName.empty())
		{
			auto it = model->m_boneIds.find(maybeBoneName);
			if (it != model->m_boneIds.end())
			{
				// this node is a bone
				node.boneId = it->second;
			}
		}

		ctx->nodes.push_back(node);

		for (size_t i = 0; i < aiNode->mNumChildren; i++)
		{
			ProcessNode(ctx, model, scene, aiNode->mChildren[i], node.id);
		}
	};

	ProcessNode(ctx, model, scene, scene->mRootNode, INVALID_ID);
}


void LoadAABoxForKeyFrame(AABoxKeyFrame* keyFrame, AnimModelLoadingCtx::CalAABBTaskParam2* param)
{
	auto ctx = param->param1->ctx;
	auto& model = param->param1->model;
	auto threadId = Thread::GetID();
	auto& taskCtx = ctx->calAABBTaskCtxs[threadId];
	auto animation = param->param1->animation;
	auto meshId = param->param1->mesh->m_model3DIdx;

	auto& channels = animation->channels;
	/*auto& tempIndex = ctx->keyFramesIndex;
	tempIndex.clear();
	tempIndex.resize(channels.size());*/

	auto& bones = taskCtx.bonesMat;
	bones.clear();
	bones.resize(model->m_boneIds.size());

	auto& vertices = ctx->animMeshesVertices[meshId].vertices;
	auto& indices = ctx->animMeshesVertices[meshId].indices;

	std::vector<Mat4>& globalTransform = taskCtx.globalTransform;
	globalTransform.clear();
	globalTransform.resize(ctx->nodes.size());

	auto& nodes = ctx->nodes;
	for (auto& node : nodes)
	{
		globalTransform[node.id] = node.localTransform;

		if (node.boneId != INVALID_ID)
		{
			auto& channel = channels[node.boneId];
			//auto& tempId = tempIndex[node.boneId];

			Mat4 scaling;
			scaling.SetScale(channel.BinaryFindScale(keyFrame->time));
			//channel.FindScaleMatrix(&scaling, 0, 0, keyFrame->time);
			Mat4 rotation;
			rotation.SetRotation(channel.BinaryFindRotation(keyFrame->time));
			//channel.FindRotationMatrix(&rotation, 0, 0, keyFrame->time);
			Mat4 translation;
			translation.SetTranslation(channel.BinaryFindTranslation(keyFrame->time));
			//channel.FindTranslationMatrix(&translation, 0, 0, keyFrame->time);

			globalTransform[node.id] = scaling * rotation * translation;
		}

		globalTransform[node.id] = globalTransform[node.id] *
			(node.parentId == INVALID_ID ? Mat4::Identity() : globalTransform[nodes[node.parentId].id]);

		/*if (node.parentId != INVALID_ID)
		{
			assert(node.id > node.parentId);
		}*/

		if (node.boneId != INVALID_ID)
		{
			bones[node.boneId] = model->m_boneOffsetMatrixs[node.boneId] * globalTransform[node.id];
		}
	}

	float
		maxX = -FLT_MAX, minX = FLT_MAX,
		maxY = -FLT_MAX, minY = FLT_MAX,
		maxZ = -FLT_MAX, minZ = FLT_MAX;

	for (auto& index : indices)
	{
		auto& vertex = vertices[index];

		Mat4 boneTransform;
		::memset(&boneTransform, 0, sizeof(Mat4));
		for (uint32_t i = 0; i < 16; i++)
		{
			boneTransform += bones[vertex.boneID[i]] * vertex.weight[i];
		}

		auto position = Vec4(vertex.position, 1.0f) * boneTransform;
		position /= position.w;

		maxX = std::max(maxX, position.x);
		minX = std::min(minX, position.x);

		maxY = std::max(maxY, position.y);
		minY = std::min(minY, position.y);

		maxZ = std::max(maxZ, position.z);
		minZ = std::min(minZ, position.z);
	}

	auto dimensions = Vec3(maxX - minX, maxY - minY, maxZ - minZ);
	auto center = Vec3(minX, minY, minZ) + dimensions / 2.0f;
	keyFrame->value = AABox(center, dimensions);
}

void LoadAABoxAnimMesh(AnimModelLoadingCtx::CalAABBTaskParam1* param, AnimModelLoadingCtx* ctx, Resource<AnimModel>& model, AnimModel::AnimMesh* animMesh, Animation* animation, const aiScene* scene)
{
	auto meshId = animMesh->m_model3DIdx;
	auto& aaBoxKeyFrames = animation->animMeshLocalAABoxKeyFrames[meshId].aaBox;

	for (auto& channel : animation->channels)
	{
		AABoxKeyFrame aaBoxKeyFrame;
		for (auto& keyFrame : channel.scaling)
		{
			aaBoxKeyFrame.time = keyFrame.time;
			auto it = std::lower_bound(aaBoxKeyFrames.begin(), aaBoxKeyFrames.end(), aaBoxKeyFrame,
				[](const AABoxKeyFrame& a, const AABoxKeyFrame& b)
				{
					return a.time < b.time;
				}
			);

			if (it != aaBoxKeyFrames.end() && it->time == keyFrame.time)
			{
				continue;
			}

			if (aaBoxKeyFrames.empty())
			{
				aaBoxKeyFrames.push_back(aaBoxKeyFrame);
			}
			else
			{
				aaBoxKeyFrames.insert(it, aaBoxKeyFrame);
			}
		}

		for (auto& keyFrame : channel.rotation)
		{
			aaBoxKeyFrame.time = keyFrame.time;
			auto it = std::lower_bound(aaBoxKeyFrames.begin(), aaBoxKeyFrames.end(), aaBoxKeyFrame,
				[](const AABoxKeyFrame& a, const AABoxKeyFrame& b)
				{
					return a.time < b.time;
				}
			);

			if (it != aaBoxKeyFrames.end() && it->time == keyFrame.time)
			{
				continue;
			}

			if (aaBoxKeyFrames.empty())
			{
				aaBoxKeyFrames.push_back(aaBoxKeyFrame);
			}
			else
			{
				aaBoxKeyFrames.insert(it, aaBoxKeyFrame);
			}
		}

		for (auto& keyFrame : channel.translation)
		{
			aaBoxKeyFrame.time = keyFrame.time;
			auto it = std::lower_bound(aaBoxKeyFrames.begin(), aaBoxKeyFrames.end(), aaBoxKeyFrame,
				[](const AABoxKeyFrame& a, const AABoxKeyFrame& b)
				{
					return a.time < b.time;
				}
			);

			if (it != aaBoxKeyFrames.end() && it->time == keyFrame.time)
			{
				continue;
			}

			if (aaBoxKeyFrames.empty())
			{
				aaBoxKeyFrames.push_back(aaBoxKeyFrame);
			}
			else
			{
				aaBoxKeyFrames.insert(it, aaBoxKeyFrame);
			}
		}
	}

	//// cal AABB of each key frame
	//std::vector<Task> tasks;
	//tasks.resize(aaBoxKeyFrames.size());
	//
	//std::vector<AnimModelLoadingCtx::CalAABBTaskParam2> taskParams;
	//taskParams.resize(aaBoxKeyFrames.size());

	//for (size_t i = 0; i < aaBoxKeyFrames.size(); i++)
	//{
	//	auto& task = tasks[i];
	//	auto& _param = taskParams[i];

	//	task.Params() = &_param;
	//	task.Entry() = [](void* p)
	//	{
	//		TASK_SYSTEM_UNPACK_PARAM_2(AnimModelLoadingCtx::CalAABBTaskParam2, p, output, param1);

	//		LoadAABoxForKeyFrame(output, param);
	//	};
	//	
	//	_param.output = &aaBoxKeyFrames[i];
	//	_param.param1 = param;

	//	//task.Entry()(task.Params());
	//}

	//TaskSystem::SubmitAndWait(tasks.data(), tasks.size(), Task::CRITICAL);

	auto threadId = Thread::GetID();
	auto& taskCtx = ctx->calAABBTaskCtxs[threadId];

	auto& bones = taskCtx.bonesMat;
	bones.clear();
	bones.resize(model->m_boneIds.size());

	auto& vertices = ctx->animMeshesVertices[meshId].vertices;
	auto& indices = ctx->animMeshesVertices[meshId].indices;

	std::vector<Mat4>& globalTransform = taskCtx.globalTransform;
	globalTransform.clear();
	globalTransform.resize(ctx->nodes.size());

	auto& channels = animation->channels;

	auto& keyFramesIndex = taskCtx.index;
	keyFramesIndex.clear();
	keyFramesIndex.resize(model->m_boneIds.size());

	for (auto& aaBoxKeyFrame : aaBoxKeyFrames)
	{
		auto& nodes = ctx->nodes;

		//::memset(keyFramesIndex.data(), 0, keyFramesIndex.size() * sizeof(KeyFramesIndex));

		for (auto& node : nodes)
		{
			globalTransform[node.id] = node.localTransform;

			if (node.boneId != INVALID_ID)
			{
				auto& channel = channels[node.boneId];
				auto& index = keyFramesIndex[node.boneId];
				//auto& tempId = tempIndex[node.boneId];

				Mat4 scaling;
				channel.FindScaleMatrix(&scaling, &index.s, index.s, aaBoxKeyFrame.time);
				Mat4 rotation;
				channel.FindRotationMatrix(&rotation, &index.r, index.r, aaBoxKeyFrame.time);
				Mat4 translation;
				channel.FindTranslationMatrix(&translation, &index.t, index.t, aaBoxKeyFrame.time);

				globalTransform[node.id] = scaling * rotation * translation;
			}

			globalTransform[node.id] = globalTransform[node.id] *
				(node.parentId == INVALID_ID ? Mat4::Identity() : globalTransform[nodes[node.parentId].id]);

			/*if (node.parentId != INVALID_ID)
			{
				assert(node.id > node.parentId);
			}*/

			if (node.boneId != INVALID_ID)
			{
				bones[node.boneId] = model->m_boneOffsetMatrixs[node.boneId] * globalTransform[node.id];
			}
		}

		float
			maxX = -FLT_MAX, minX = FLT_MAX,
			maxY = -FLT_MAX, minY = FLT_MAX,
			maxZ = -FLT_MAX, minZ = FLT_MAX;

		for (auto& index : indices)
		{
			auto& vertex = vertices[index];

			Mat4 boneTransform;
			::memset(&boneTransform, 0, sizeof(Mat4));
			for (uint32_t i = 0; i < 16; i++)
			{
				boneTransform += bones[vertex.boneID[i]] * vertex.weight[i];
			}

			auto position = Vec4(vertex.position, 1.0f) * boneTransform;
			position /= position.w;

			maxX = std::max(maxX, position.x);
			minX = std::min(minX, position.x);

			maxY = std::max(maxY, position.y);
			minY = std::min(minY, position.y);

			maxZ = std::max(maxZ, position.z);
			minZ = std::min(minZ, position.z);
		}

		auto dimensions = Vec3(maxX - minX, maxY - minY, maxZ - minZ);
		auto center = Vec3(minX, minY, minZ) + dimensions / 2.0f;
		aaBoxKeyFrame.value = AABox(center, dimensions);
	}

	auto boundAABox = aaBoxKeyFrames[0].value;
	for (auto& keyFrame : aaBoxKeyFrames)
	{
		boundAABox.Joint(keyFrame.value);
	}

	animation->animMeshLocalAABoxKeyFrames[meshId].boundAABox = boundAABox;
}


void LoadAnimModelHierarchy(AnimModelLoadingCtx* ctx, GameObject* obj, Resource<AnimModel>& model, std::vector<Resource<Texture2D>>& diffuseTextures, const aiScene* scene)
{
	constexpr static auto ProcessNonAnimMesh =
		[](AnimModelLoadingCtx* ctx, size_t i, GameObject* obj, Resource<AnimModel>& model, std::vector<Resource<Texture2D>>& diffuseTextures, aiMesh* aiMesh, aiNode* node) -> void
	{
		auto comp = obj->NewComponent<MeshBasicRenderer>(false);

		comp->m_model3D = model;
		comp->m_mesh = (Model3DBasic::Mesh*)ctx->meshes[node->mMeshes[i]];

		if (aiMesh->mMaterialIndex >= 0)
		{
			comp->m_texture = diffuseTextures[aiMesh->mMaterialIndex];
		}
		else
		{
			comp->m_texture = diffuseTextures.back();
		}
	};

	constexpr static auto ProcessAnimMesh =
		[](AnimModelLoadingCtx* ctx, size_t i, GameObject* obj, Resource<AnimModel>& model, std::vector<Resource<Texture2D>>& diffuseTextures, aiMesh* aiMesh, aiNode* node) -> void
	{
		auto comp = obj->NewComponent<AnimMeshRenderer>();

		comp->m_model3D = model;
		comp->m_mesh = (AnimModel::AnimMesh*)ctx->meshes[node->mMeshes[i]];

		comp->m_animMeshRenderingBuffer = ctx->animMeshRenderingBuffer;

		ctx->animator->m_animMeshRendererObjs[comp->m_mesh->m_model3DIdx] = obj;

		if (aiMesh->mMaterialIndex >= 0)
		{
			comp->m_texture = diffuseTextures[aiMesh->mMaterialIndex];
		}
		else
		{
			comp->m_texture = diffuseTextures.back();
		}
	};

	constexpr static void (*ProcessNode)(AnimModelLoadingCtx*, GameObject*, Resource<AnimModel>&, std::vector<Resource<Texture2D>>&, const aiScene*, aiNode* ) =
		[](AnimModelLoadingCtx* ctx, GameObject* obj, Resource<AnimModel>& model, std::vector<Resource<Texture2D>>& diffuseTextures, const aiScene* scene, aiNode* node) -> void
	{
		String maybeBoneName = node->mName.C_Str();
		if (!maybeBoneName.empty())
		{
			obj->Name() = maybeBoneName;
			ctx->objectMap.insert({ maybeBoneName,obj });

			auto it = model->m_boneIds.find(maybeBoneName);
			if (it != model->m_boneIds.end())
			{
				// this node is a bone

				auto comp = obj->NewComponent<AnimSkeletalGameObject>();
				comp->m_model3D = model;
				comp->m_boneId = it->second;
				comp->m_animMeshRenderingBuffer = ctx->animMeshRenderingBuffer;
				comp->m_animator = ctx->animator;
			}
		}

		size_t animMeshCount = 0;
		if (node->mNumMeshes > 1)
		{
			auto compoundObj = mheap::New<GameObject>();
			for (size_t i = 0; i < node->mNumMeshes; i++)
			{
				auto aiMesh = scene->mMeshes[node->mMeshes[i]];
				auto child = mheap::New<GameObject>();
				child->Name() = aiMesh->mName.C_Str();

				compoundObj->AddChild(child);

				if (!aiMesh->HasBones())
				{
					ProcessNonAnimMesh(ctx, i, child, model, diffuseTextures, aiMesh, node);
					continue;
				}
				
				ProcessAnimMesh(ctx, i, child, model, diffuseTextures, aiMesh, node);

				animMeshCount++;
			}

			obj->AddChild(compoundObj);
		}
		else if (node->mNumMeshes == 1)
		{
			auto aiMesh = scene->mMeshes[node->mMeshes[0]];

			if (!aiMesh->HasBones())
			{
				ProcessNonAnimMesh(ctx, 0, obj, model, diffuseTextures, aiMesh, node);
			}
			else
			{
				ProcessAnimMesh(ctx, 0, obj, model, diffuseTextures, aiMesh, node);
				animMeshCount++;
			}

			//obj->Name() = aiMesh->mName.C_Str();
		}

		aiVector3D scale;
		aiQuaternion rot;
		aiVector3D pos;

		if (!animMeshCount)
		{
			node->mTransformation.Decompose(scale, rot, pos);
		}
		else
		{
			//auto mat = node->mParent->mTransformation;
			auto mat = scene->mRootNode->mTransformation;
			mat.Inverse().Decompose(scale, rot, pos);
		}

		Transform transform = {};
		transform.Scale() = reinterpret_cast<const Vec3&>(scale);
		transform.Rotation() = { rot.w, rot.x, rot.y, rot.z };
		transform.Position() = reinterpret_cast<const Vec3&>(pos);

		obj->SetLocalTransform(transform);
		
		for (size_t i = 0; i < node->mNumChildren; i++)
		{
			auto child = mheap::New<GameObject>();
			obj->AddChild(child);
			ProcessNode(ctx, child, model, diffuseTextures, scene, node->mChildren[i]);
		}
	};

	ProcessNode(ctx, obj, model, diffuseTextures, scene, scene->mRootNode);
}

void AnimModelCreateCache(AnimModelLoadingCtx* ctx, AnimModel* model, ByteStream& stream, const String& streamPath)
{
	{
		// write aabb cache

		auto& animations = model->m_animations;
		stream.Put(animations.size());

		for (auto& animation : animations)
		{
			stream.Put(animation.animMeshLocalAABoxKeyFrames.size());

			for (auto& aabbKeyFrames : animation.animMeshLocalAABoxKeyFrames)
			{
				stream.Put(aabbKeyFrames.aaBox.size());

				for (auto& aabbKeyFrame : aabbKeyFrames.aaBox)
				{
					stream.Put(aabbKeyFrame);
				}

				stream.Put(aabbKeyFrames.boundAABox);
			}
		}
	}

	FileSystem::Get()->WriteStream(streamPath.c_str(), &stream);
}

void AnimModelReadCache(AnimModelLoadingCtx* ctx, AnimModel* model, ByteStream& stream)
{
	{
		// read aabb cache

		auto& animations = model->m_animations;

		size_t temp;
		stream.Pick(temp);
		animations.resize(temp);

		for (auto& animation : animations)
		{
			stream.Pick(temp);
			animation.animMeshLocalAABoxKeyFrames.resize(temp);

			for (auto& aabbKeyFrames : animation.animMeshLocalAABoxKeyFrames)
			{
				stream.Pick(temp);
				aabbKeyFrames.aaBox.resize(temp);

				for (auto& aabbKeyFrame : aabbKeyFrames.aaBox)
				{
					stream.Pick(aabbKeyFrame);
				}

				stream.Pick(aabbKeyFrames.boundAABox);
			}
		}
	}
}

void LoadMaterialsForAnimModel(const String& basePath, const String& defaultDiffusePath, std::vector<Resource<Texture2D>>& diffuseTextures, const aiScene* scene)
{
	auto fs = FileSystem::Get();

	if (scene->HasMaterials())
	{
		auto* materials = scene->mMaterials;
		auto materialsCount = scene->mNumMaterials;

		for (size_t i = 0; i < materialsCount; i++)
		{
			auto material = materials[i];

			aiString file;
			//material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), file);
			material->GetTexture(aiTextureType_DIFFUSE, 0, &file);

			std::string str = file.C_Str();
			std::replace(str.begin(), str.end(), '\\', '/');
			String diffusePath = fs->GetResourcesRelativePath(basePath + str.c_str());

			if (fs->IsResourceExist(diffusePath.c_str()))
			{
				diffuseTextures.push_back(resource::Load<Texture2D>(diffusePath.c_str()));
			}
			else
			{
				diffuseTextures.push_back(resource::Load<Texture2D>(defaultDiffusePath));
			}
		}
	}

	diffuseTextures.push_back(resource::Load<Texture2D>(defaultDiffusePath));

}

void CreateAABoxKeyFramesForAnimModel(String path, AnimModelLoadingCtx& ctx, Resource<AnimModel>& model3D, const aiScene* scene)
{
	ByteStream stream;
	path = "Resources/" + path;
	auto streamPath = (path + AnimModel::CACHE_EXTENSION);
	if (FileSystem::Get()->IsFileChanged(path.c_str()) || !FileSystem::Get()->ReadStream(streamPath.c_str(), &stream))
	{
		if (ctx.nodes.empty())
		{
			FlattenAnimModelHierarchy(&ctx, model3D, scene);
		}

		//size_t count = 0;

		auto numTasks = model3D->m_animMeshes.size() * model3D->m_animations.size();
		std::vector<Task> tasks;
		tasks.resize(numTasks);

		std::vector<AnimModelLoadingCtx::CalAABBTaskParam1> taskParams;
		taskParams.resize(numTasks);

		ctx.calAABBTaskCtxs.resize(TaskSystem::GetWorkerCount());

		size_t count = 0;
		for (auto& animMesh : model3D->m_animMeshes)
		{
			for (auto& animation : model3D->m_animations)
			{
				//LoadAABoxAnimMesh(&ctx, model3D, &animMesh, &animation, scene);
				//std::cout << count << "\n";
				//if (count++ == 4) break;
				//break;

				auto& task = tasks[count];
				auto& param = taskParams[count];

				task.Params() = &param;
				task.Entry() = [](void* p)
				{
					TASK_SYSTEM_UNPACK_PARAM_REF_5(AnimModelLoadingCtx::CalAABBTaskParam1, p, ctx, animation, mesh, model, scene);

					LoadAABoxAnimMesh(param, ctx, model, mesh, animation, scene);
				};

				param.animation = &animation;
				param.ctx = &ctx;
				param.mesh = &animMesh;
				param.model = model3D;
				param.scene = scene;

				//task.Entry()(task.Params());

				count++;
			}

			//std::cout << count << "\n";
			//if (count++ == 10) break;
		}

		TaskSystem::SubmitAndWait(tasks.data(), tasks.size(), Task::CRITICAL);

		AnimModelCreateCache(&ctx, model3D, stream, streamPath);
	}
	else
	{
		AnimModelReadCache(&ctx, model3D, stream);
	}
}

Handle<GameObject> LoadAnimModel(String path, String defaultDiffusePath)
{
	auto fs = FileSystem::Get();

	if (defaultDiffusePath.empty())
	{
		defaultDiffusePath = Texture2D::DEFAULT_FILE;
	}

	auto model3D = resource::Load<AnimModel>(path, true);

	auto ret = Runtime::Get()->GameObjectCache()->Get("AnimatorSkeletalGameObject|" + model3D->GetPath());
	if (ret.Get())
	{
		Serializer serializer;
		return StaticCast<GameObject>(serializer.Clone(ret.Get()));
	}

	ret = mheap::New<GameObject>();

	auto animator = ret->NewComponent<AnimatorSkeletalGameObject>();

	std::vector<Resource<Texture2D>> diffuseTextures;

	std::string_view pathview(path.c_str());
	String basePath = path.SubString(0, pathview.find_last_of('/') + 1);

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fs->GetResourcesPath(path).c_str(),
		aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded);

	LoadMaterialsForAnimModel(basePath, defaultDiffusePath, diffuseTextures, scene);
	
	if (model3D->m_meshes.size() == 0 && model3D->m_animMeshes.size() == 0)
	{
		LoadAllMeshsForModel3DBasic(model3D, scene, false);

		LoadAllAnimMeshsForAnimModel(model3D, scene);
	}

	AnimModelLoadingCtx ctx(model3D, scene);

	ctx.animator = animator.Get();
	ctx.animator->m_animMeshRendererObjs.Resize(model3D->m_animMeshes.size());
	ctx.animator->m_model3D = model3D;

	LoadAnimModelHierarchy(&ctx, ret, model3D, diffuseTextures, scene);

	if (model3D->m_animations.size() == 0)
	{
		LoadAnimModelAnimation(&ctx, model3D, scene);

		CreateAABoxKeyFramesForAnimModel(path, ctx, model3D, scene);
	}

	ctx.animator->m_animationId = 0;
	ctx.animator->m_ticksPerSecond = model3D->m_animations[0].ticksPerSecond;
	ctx.animator->m_tickDuration = model3D->m_animations[0].tickDuration;

	Runtime::Get()->GameObjectCache()->Store("AnimatorSkeletalGameObject|" + model3D->GetPath(), ret);

	Serializer serializer;
	return StaticCast<GameObject>(serializer.Clone(ret.Get()));
}

void LoadAnimModelHierarchyArray(AnimModelLoadingCtx* ctx, GameObject* obj, Resource<AnimModel>& model, std::vector<Resource<Texture2D>>& diffuseTextures, const aiScene* scene)
{
	constexpr static auto ProcessNonAnimMesh =
		[](AnimModelLoadingCtx* ctx, size_t i, GameObject* obj, Resource<AnimModel>& model, std::vector<Resource<Texture2D>>& diffuseTextures, aiMesh* aiMesh, aiNode* node, ID nodeId, Transform& transform) -> void
	{
		auto comp = obj->NewComponent<AnimModelStaticMeshRenderer>(false);

		comp->m_model3D = model;
		comp->m_mesh = (Model3DBasic::Mesh*)ctx->meshes[node->mMeshes[i]];

		if (aiMesh->mMaterialIndex >= 0)
		{
			comp->m_texture = diffuseTextures[aiMesh->mMaterialIndex];
		}
		else
		{
			comp->m_texture = diffuseTextures.back();
		}
	};

	constexpr static auto ProcessAnimMesh =
		[](AnimModelLoadingCtx* ctx, size_t i, GameObject* obj, Resource<AnimModel>& model, std::vector<Resource<Texture2D>>& diffuseTextures, aiMesh* aiMesh, aiNode* node, ID nodeId, Transform& transform) -> void
	{
		auto comp = obj->NewComponent<AnimMeshRenderer>();

		comp->m_model3D = model;
		comp->m_mesh = (AnimModel::AnimMesh*)ctx->meshes[node->mMeshes[i]];

		comp->m_animMeshRenderingBuffer = ctx->animMeshRenderingBuffer;

		//ctx->animatorArray->m_meshRendererObjs[comp->m_mesh->m_model3DIdx] = obj;

		if (aiMesh->mMaterialIndex >= 0)
		{
			comp->m_texture = diffuseTextures[aiMesh->mMaterialIndex];
		}
		else
		{
			comp->m_texture = diffuseTextures.back();
		}

		auto animator = ctx->animatorArray;
		animator->m_meshRendererObjs.Push(obj);
		model->m_boundNodeIds.push_back({ INVALID_ID });

		obj->SetLocalTransform(transform);
	};

	constexpr static void (*ProcessNode)(AnimModelLoadingCtx*, GameObject*, Resource<AnimModel>&, std::vector<Resource<Texture2D>>&, const aiScene*, aiNode*, ID&) =
		[](AnimModelLoadingCtx* ctx, GameObject* obj, Resource<AnimModel>& model, std::vector<Resource<Texture2D>>& diffuseTextures, const aiScene* scene, aiNode* node, ID& nodeId)-> void
	{
		size_t animMeshCount = 0;
		aiVector3D scale;
		aiQuaternion rot;
		aiVector3D pos;

		//if (!animMeshCount)
		//{
			node->mTransformation.Decompose(scale, rot, pos);
		//}
		//else
		//{
		//	//auto mat = node->mParent->mTransformation;
		//	auto mat = scene->mRootNode->mTransformation;
		//	mat.Inverse().Decompose(scale, rot, pos);
		//}

		Transform transform = {};
		transform.Scale() = reinterpret_cast<const Vec3&>(scale);
		transform.Rotation() = { rot.w, rot.x, rot.y, rot.z };
		transform.Position() = reinterpret_cast<const Vec3&>(pos);

		if (node->mNumMeshes > 1)
		{
			auto compoundObj = mheap::New<GameObject>();

			bool hasStaticMesh = false;

			for (size_t i = 0; i < node->mNumMeshes; i++)
			{
				auto aiMesh = scene->mMeshes[node->mMeshes[i]];
				auto child = mheap::New<GameObject>();
				child->Name() = aiMesh->mName.C_Str();

				compoundObj->AddChild(child);

				if (!aiMesh->HasBones())
				{
					hasStaticMesh = true;
					ProcessNonAnimMesh(ctx, i, child, model, diffuseTextures, aiMesh, node, nodeId, transform);
					continue;
				}

				auto mat = scene->mRootNode->mTransformation;
				mat.Inverse().Decompose(scale, rot, pos);
				transform.Scale() = reinterpret_cast<const Vec3&>(scale);
				transform.Rotation() = { rot.w, rot.x, rot.y, rot.z };
				transform.Position() = reinterpret_cast<const Vec3&>(pos);

				ProcessAnimMesh(ctx, i, child, model, diffuseTextures, aiMesh, node, nodeId, transform);

				animMeshCount++;
			}

			if (hasStaticMesh)
			{
				auto animator = ctx->animatorArray;
				animator->m_meshRendererObjs.Push(compoundObj);
				model->m_boundNodeIds.push_back({ nodeId });
			}

			obj->AddChild(compoundObj);
		}
		else if (node->mNumMeshes == 1)
		{
			auto compoundObj = mheap::New<GameObject>();
			auto aiMesh = scene->mMeshes[node->mMeshes[0]];

			if (!aiMesh->HasBones())
			{
				ProcessNonAnimMesh(ctx, 0, compoundObj, model, diffuseTextures, aiMesh, node, nodeId, transform);
				auto animator = ctx->animatorArray;
				animator->m_meshRendererObjs.Push(compoundObj);
				model->m_boundNodeIds.push_back({ nodeId });
			}
			else
			{
				auto mat = scene->mRootNode->mTransformation;
				mat.Inverse().Decompose(scale, rot, pos);
				transform.Scale() = reinterpret_cast<const Vec3&>(scale);
				transform.Rotation() = { rot.w, rot.x, rot.y, rot.z };
				transform.Position() = reinterpret_cast<const Vec3&>(pos);

				ProcessAnimMesh(ctx, 0, compoundObj, model, diffuseTextures, aiMesh, node, nodeId, transform);
				animMeshCount++;
			}

			obj->AddChild(compoundObj);
		}

		nodeId++;
		for (size_t i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(ctx, obj, model, diffuseTextures, scene, node->mChildren[i], nodeId);
		}
	};

	size_t nodeId = 0;
	ProcessNode(ctx, obj, model, diffuseTextures, scene, scene->mRootNode, nodeId);
}

Handle<GameObject> LoadAnimModelArray(String path, String defaultDiffusePath)
{
	auto fs = FileSystem::Get();

	if (defaultDiffusePath.empty())
	{
		defaultDiffusePath = Texture2D::DEFAULT_FILE;
	}

	auto model3D = resource::Load<AnimModel>(path, true);

	auto ret = Runtime::Get()->GameObjectCache()->Get("AnimatorSkeletalArray|" + model3D->GetPath());
	if (ret.Get())
	{
		Serializer serializer;
		return StaticCast<GameObject>(serializer.Clone(ret.Get()));
	}

	ret = mheap::New<GameObject>();

	auto animator = ret->NewComponent<AnimatorSkeletalArray>();

	std::vector<Resource<Texture2D>> diffuseTextures;

	std::string_view pathview(path.c_str());
	String basePath = path.SubString(0, pathview.find_last_of('/') + 1);

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fs->GetResourcesPath(path).c_str(),
		aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded);

	LoadMaterialsForAnimModel(basePath, defaultDiffusePath, diffuseTextures, scene);

	if (model3D->m_meshes.size() == 0 && model3D->m_animMeshes.size() == 0)
	{
		LoadAllMeshsForModel3DBasic(model3D, scene, false);

		LoadAllAnimMeshsForAnimModel(model3D, scene);
	}

	AnimModelLoadingCtx ctx(model3D, scene);

	ctx.animatorArray = animator.Get();
	//ctx.animatorArray->m_meshRendererObjs.Resize(model3D->m_animMeshes.size());
	ctx.animatorArray->m_model3D = model3D;
	ctx.animatorArray->m_animMeshRenderingBuffer = ctx.animMeshRenderingBuffer;

	FlattenAnimModelHierarchy(&ctx, model3D, scene);

	LoadAnimModelHierarchyArray(&ctx, ret, model3D, diffuseTextures, scene);

	if (model3D->m_animations.size() == 0)
	{
		LoadAnimModelAnimation(&ctx, model3D, scene);

		CreateAABoxKeyFramesForAnimModel(path, ctx, model3D, scene);
	}

	ctx.animatorArray->m_keyFramesIndex.resize(model3D->m_boneIds.size());

	auto& srcNodes = ctx.nodes;
	auto& destNodes = model3D->m_nodes;
	if (destNodes.empty())
	{
		destNodes.resize(srcNodes.size());
		for (size_t i = 0; i < srcNodes.size(); i++)
		{
			auto& srcNode = srcNodes[i];
			auto& destNode = destNodes[i];

			destNode.boneId = srcNode.boneId;
			destNode.parentId = srcNode.parentId;
			destNode.localTransform = srcNode.localTransform;
		}
	}

	ctx.animatorArray->m_globalTransforms.resize(destNodes.size());
	ctx.animatorArray->m_animationId = 0;
	ctx.animatorArray->m_ticksPerSecond = model3D->m_animations[0].ticksPerSecond;
	ctx.animatorArray->m_tickDuration = model3D->m_animations[0].tickDuration;

	aiVector3D scale;
	aiQuaternion rot;
	aiVector3D pos;
	scene->mRootNode->mTransformation.Decompose(scale, rot, pos);
	Transform transform = {};
	transform.Scale() = reinterpret_cast<const Vec3&>(scale);
	transform.Rotation() = { rot.w, rot.x, rot.y, rot.z };
	transform.Position() = reinterpret_cast<const Vec3&>(pos);

	ret->SetLocalTransform(transform);

	Runtime::Get()->GameObjectCache()->Store("AnimatorSkeletalArray|" + model3D->GetPath(), ret);

	Serializer serializer;
	return StaticCast<GameObject>(serializer.Clone(ret.Get()));
}

}

NAMESPACE_END