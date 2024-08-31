#include "Utils.h"

#include "../AnimModel.h"

#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"
#include "MainSystem/Rendering/Components/AnimModelStaticMeshRenderer.h"
#include "MainSystem/Rendering/Components/AnimMeshRenderer.h"
#include "MainSystem/Animation/Components/AnimSkeletalGameObject.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalGameObject.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"
#include "MainSystem/Animation/AnimLayer/AnimPlayerLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimBlendLayer.h"

#include "Runtime/Runtime.h"
#include "Scene/GameObjectCache.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

NAMESPACE_BEGIN

namespace ResourceUtils
{
extern void LoadAllMeshsForModel3DBasic(Model3DBasic* model3D, const aiScene* scene, bool ignoreBones);

extern void LoadAnimMotion(String, void*, std::vector<Resource<AnimMotion>>&);

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

	using AnimMeshVertices = AnimModel::AnimMeshVertices;

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

	inline AnimModelLoadingCtx(AnimModel* model, const aiScene* scene, bool initAnimMeshRenderingBuffer = true)
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

		if (initAnimMeshRenderingBuffer)
		{
			AnimModel::AnimMeshRenderingBufferData buffer;
			buffer.bones.resize(model->m_boneIds.size());
			buffer.meshesAABB.resize(model->m_animMeshes.size());
			animMeshRenderingBuffer = std::make_shared<AnimModel::AnimMeshRenderingBuffer>();
			animMeshRenderingBuffer->buffer.Initialize(buffer);
		}

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

		std::bitset<std::numeric_limits<uint16_t>::max()> hasBoneIds;

		while (vertexTypeIdx < NUM_TYPE)
		{
			for (uint32_t i = 0; i < mesh->mNumBones; i++)
			{
				auto bone = mesh->mBones[i];
				String name = bone->mName.C_Str();

				assert(model->m_boneIds.find(name) != model->m_boneIds.end());

				auto boneId = uint16_t(model->m_boneIds[name]);

				if (!hasBoneIds.test(boneId))
				{
					output->m_influencedByBoneIds.push_back(boneId);
				}

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
		output->m_vertexBuffer->UpdateBuffer(verticesBuffer.data(), vbDesc.count * vbDesc.stride, {});

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

				auto m = ConvertAssimpMat4(bone->mOffsetMatrix);
				model->m_boneOffsetMatrixs.push_back(m);
				model->m_boneOffsetInvMatrixs.push_back(m.GetInverse());
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

void LoadAnimMeshVertices(AnimModelLoadingCtx::AnimMeshVertices* animMeshVertices, AnimModel* model, aiMesh* mesh)
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

void FlattenAnimModelHierarchy(AnimModelLoadingCtx* ctx, AnimModel* model, const aiScene* scene)
{
	constexpr static void (*ProcessNode)(AnimModelLoadingCtx*, AnimModel*, const aiScene*, aiNode*, ID) =
		[](AnimModelLoadingCtx* ctx, AnimModel* model, const aiScene* scene, aiNode* aiNode, ID parentId) -> void
	{
		for (size_t i = 0; i < aiNode->mNumMeshes; i++)
		{
			auto aiMesh = scene->mMeshes[aiNode->mMeshes[i]];
			if (!aiMesh->HasBones())
			{
				continue;
			}

			auto animMeshId = ((AnimModel::AnimMesh*)ctx->meshes[aiNode->mMeshes[i]])->m_model3DIdx;
			LoadAnimMeshVertices(&ctx->animMeshesVertices[animMeshId], model, aiMesh);
		}

		AnimModelLoadingCtx::Node node = {};
		node.id = ctx->nodes.size();
		node.parentId = parentId;
		node.localTransform = ConvertAssimpMat4(aiNode->mTransformation);
		
		String maybeBoneName = aiNode->mName.C_Str();
		if (!maybeBoneName.empty())
		{
			String name = maybeBoneName;
			auto it = model->m_boneIds.find(maybeBoneName);
			if (it != model->m_boneIds.end())
			{
				// this node is a bone
				node.boneId = it->second;
				name = it->first;
			}

			model->m_nodeIds[name] = ctx->nodes.size();
		}

		ctx->nodes.push_back(node);

		for (size_t i = 0; i < aiNode->mNumChildren; i++)
		{
			ProcessNode(ctx, model, scene, aiNode->mChildren[i], node.id);
		}
	};

	ProcessNode(ctx, model, scene, scene->mRootNode, INVALID_ID);
}

void LoadMaterialsForAnimModel(const String& basePath, std::vector<String>& diffuseTextures, const aiScene* scene)
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
			String diffusePath = basePath + str.c_str();// fs->GetResourcesRelativePath(basePath + str.c_str());
			//assert(0 && "TODO: Check this again!!!");

			if (fs->IsFileExisted(diffusePath.c_str()))
			{
				diffuseTextures.push_back(diffusePath);
			}
			else
			{
				diffuseTextures.push_back({});
			}
		}
	}

	diffuseTextures.push_back({});

}

//void CreateAABoxKeyFramesForAnimModel(String path, AnimModelLoadingCtx& ctx, Resource<AnimModel>& model3D, const aiScene* scene)
//{
//	ByteStream stream;
//	path = "Resources/" + path;
//	auto streamPath = (path + AnimModel::CACHE_EXTENSION);
//	if (FileSystem::Get()->IsFileChanged(path.c_str()) || !FileSystem::Get()->ReadStream(streamPath.c_str(), &stream))
//	{
//		if (ctx.nodes.empty())
//		{
//			FlattenAnimModelHierarchy(&ctx, model3D, scene);
//		}
//
//		//size_t count = 0;
//
//		auto numTasks = model3D->m_animMeshes.size() * model3D->m_animations.size();
//		std::vector<Task> tasks;
//		tasks.resize(numTasks);
//
//		std::vector<AnimModelLoadingCtx::CalAABBTaskParam1> taskParams;
//		taskParams.resize(numTasks);
//
//		ctx.calAABBTaskCtxs.resize(TaskSystem::GetWorkerCount());
//
//		size_t count = 0;
//		for (auto& animMesh : model3D->m_animMeshes)
//		{
//			for (auto& animation : model3D->m_animations)
//			{
//				//LoadAABoxAnimMesh(&ctx, model3D, &animMesh, &animation, scene);
//				//std::cout << count << "\n";
//				//if (count++ == 4) break;
//				//break;
//
//				auto& task = tasks[count];
//				auto& param = taskParams[count];
//
//				task.Params() = &param;
//				task.Entry() = [](void* p)
//				{
//					TASK_SYSTEM_UNPACK_PARAM_REF_5(AnimModelLoadingCtx::CalAABBTaskParam1, p, ctx, animation, mesh, model, scene);
//
//					LoadAABoxAnimMesh(param, ctx, model, mesh, animation, scene);
//				};
//
//				param.animation = &animation;
//				param.ctx = &ctx;
//				param.mesh = &animMesh;
//				param.model = model3D;
//				param.scene = scene;
//
//				//task.Entry()(task.Params());
//
//				count++;
//			}
//
//			//std::cout << count << "\n";
//			//if (count++ == 10) break;
//		}
//
//		TaskSystem::SubmitAndWait(tasks.data(), tasks.size(), Task::CRITICAL);
//
//		AnimModelCreateCache(&ctx, model3D, stream, streamPath);
//	}
//	else
//	{
//		AnimModelReadCache(&ctx, model3D, stream);
//	}
//}

//Handle<GameObject> LoadAnimModel(String path, String defaultDiffusePath)
//{
//	auto fs = FileSystem::Get();
//
//	if (defaultDiffusePath.empty())
//	{
//		defaultDiffusePath = Texture2D::DEFAULT_FILE;
//	}
//
//	auto model3D = resource::Load<AnimModel>(path, true);
//
//	auto ret = Runtime::Get()->GameObjectCache()->Get("AnimatorSkeletalGameObject|" + model3D->GetPath());
//	if (ret.Get())
//	{
//		Serializer serializer;
//		return StaticCast<GameObject>(serializer.Clone(ret));
//	}
//
//	ret = mheap::New<GameObject>();
//
//	auto animator = ret->NewComponent<AnimatorSkeletalGameObject>();
//
//	std::vector<Resource<Texture2D>> diffuseTextures;
//
//	std::string_view pathview(path.c_str());
//	String basePath = path.SubString(0, pathview.find_last_of('/') + 1);
//
//	Assimp::Importer importer;
//	const aiScene* scene = importer.ReadFile(fs->GetResourcesPath(path).c_str(),
//		aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded);
//
//	//LoadMaterialsForAnimModel(basePath, diffuseTextures, scene);
//	
//	if (model3D->m_meshes.size() == 0 && model3D->m_animMeshes.size() == 0)
//	{
//		LoadAllMeshsForModel3DBasic(model3D, scene, false);
//
//		LoadAllAnimMeshsForAnimModel(model3D, scene);
//	}
//
//	AnimModelLoadingCtx ctx(model3D, scene);
//
//	ctx.animator = animator.Get();
//	ctx.animator->m_animMeshRendererObjs.Resize(model3D->m_animMeshes.size());
//	ctx.animator->m_model3D = model3D;
//
//	LoadAnimModelHierarchy(&ctx, ret, model3D, diffuseTextures, scene);
//
//	if (model3D->m_animations.size() == 0)
//	{
//		//LoadAnimModelAnimation(&ctx, model3D, scene);
//
//		//CreateAABoxKeyFramesForAnimModel(path, ctx, model3D, scene);
//
//		std::vector<Resource<AnimMotion>> motions;
//		LoadAnimMotion(path, (void*)scene, motions);
//
//		for (auto& motion : motions)
//		{
//			model3D->AddAnimation(motion);
//		}
//	}
//
//	ctx.animator->m_animationId = 0;
//	ctx.animator->m_ticksPerSecond = model3D->m_animations[0]->GetTicksPerSecond();
//	ctx.animator->m_tickDuration = model3D->m_animations[0]->GetTickDuration();
//	ctx.animator->m_aabbKeyFrameIndex.resize(model3D->m_animMeshes.size());
//
//	Runtime::Get()->GameObjectCache()->Store("AnimatorSkeletalGameObject|" + model3D->GetPath(), ret);
//
//	Serializer serializer;
//	return StaticCast<GameObject>(serializer.Clone(ret));
//}

}

NAMESPACE_END