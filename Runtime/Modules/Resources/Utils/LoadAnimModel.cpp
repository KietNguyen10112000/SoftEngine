#include "Utils.h"

#include "../AnimModel.h"

#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"
#include "MainSystem/Rendering/Components/AnimMeshRenderer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

NAMESPACE_BEGIN

namespace ResourceUtils
{
extern void LoadAllMeshsForModel3DBasic(Model3DBasic* model3D, const aiScene* scene, bool ignoreBones);

struct AnimModelLoadingCtx
{
	// refer to m_animMeshes/m_meshes
	std::vector<void*> meshes;

	SharedPtr<AnimModel::BoneShaderBuffer> shaderBuffer;
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

		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace& face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; j++)
			{
				auto idx = face.mIndices[j];
				auto& aiVertex = aiVertices[idx];

				auto& vertex = *(AnimModel::AnimVertex::WeightVertex_16*)(verticesBuffer.data() + idx * vertexTypeSize);

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

		comp->m_shaderBuffer = ctx->shaderBuffer;

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
		size_t animMeshCount = 0;
		if (node->mNumMeshes > 1)
		{
			auto compoundObj = mheap::New<GameObject>();
			for (size_t i = 0; i < node->mNumMeshes; i++)
			{
				auto aiMesh = scene->mMeshes[node->mMeshes[i]];
				auto child = mheap::New<GameObject>();
				child->Name() = aiMesh->mName.C_Str();

				if (!aiMesh->HasBones())
				{
					ProcessNonAnimMesh(ctx, i, child, model, diffuseTextures, aiMesh, node);
					compoundObj->AddChild(child);
					continue;
				}

				
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
			}

			obj->Name() = aiMesh->mName.C_Str();
		}

		aiVector3D scale;
		aiQuaternion rot;
		aiVector3D pos;
		node->mTransformation.Decompose(scale, rot, pos);

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

Handle<GameObject> LoadAnimModel(String path, String defaultDiffusePath)
{
	auto fs = FileSystem::Get();

	if (defaultDiffusePath.empty())
	{
		defaultDiffusePath = Texture2D::DEFAULT_FILE;
	}

	auto ret = mheap::New<GameObject>();
	auto model3D = resource::Load<AnimModel>(path, true);

	std::vector<Resource<Texture2D>> diffuseTextures;

	std::string_view pathview(path.c_str());
	String basePath = path.SubString(0, pathview.find_last_of('/') + 1);

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fs->GetResourcesPath(path).c_str(),
		aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded);

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

	LoadAllMeshsForModel3DBasic(model3D, scene, false);

	LoadAllAnimMeshsForAnimModel(model3D, scene);

	AnimModelLoadingCtx ctx;

	constexpr static auto PrepareCtx = [](AnimModelLoadingCtx* ctx, AnimModel* model, const aiScene* scene)
	{
		auto& nonAnimMeshes = model->m_meshes;
		auto& animMeshes = model->m_animMeshes;
		uint32_t nonAnimMeshCount = 0;
		uint32_t animMeshCount = 0;
		ctx->meshes.resize(scene->mNumMeshes);

		for (uint32_t i = 0; i < scene->mNumMeshes; i++)
		{
			auto aiMesh = scene->mMeshes[i];

			if (!aiMesh->HasBones())
			{
				ctx->meshes[i] = &nonAnimMeshes[nonAnimMeshCount];
				nonAnimMeshCount++;
				continue;
			}

			ctx->meshes[i] = &animMeshes[animMeshCount];
			animMeshCount++;
		}

		ctx->shaderBuffer = std::make_shared<AnimModel::BoneShaderBuffer>();
		ctx->shaderBuffer->bones.resize(model->m_boneIds.size());
	};

	PrepareCtx(&ctx, model3D, scene);

	LoadAnimModelHierarchy(&ctx, ret, model3D, diffuseTextures, scene);

	return ret;
}

}

NAMESPACE_END