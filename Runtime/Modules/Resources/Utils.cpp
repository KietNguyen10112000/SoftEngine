#include "Utils.h"

#include "Model3DBasic.h"
#include "MeshBasic.h"

#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

NAMESPACE_BEGIN

Handle<GameObject> ResourceUtils::LoadModel3DBasic(String path)
{
	auto fs = FileSystem::Get();

	auto ret = mheap::New<GameObject>();
	auto model3D = resource::Load<Model3DBasic>(path);

	std::vector<Resource<Texture2D>> diffuseTextures;

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fs->GetResourcesPath(path).c_str(), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

	constexpr static void (*ProcessNode)(GameObject*, Resource<Model3DBasic>&, std::vector<Resource<Texture2D>>&, const aiScene*, aiNode*) =
		[](GameObject* obj, Resource<Model3DBasic>& model, std::vector<Resource<Texture2D>>& diffuseTextures, const aiScene* scene, aiNode* node) {

		if (node->mNumMeshes > 1)
		{
			auto compoundObj = mheap::New<GameObject>();
			for (size_t i = 0; i < node->mNumMeshes; i++)
			{
				auto aiMesh = scene->mMeshes[node->mMeshes[i]];
				auto child = mheap::New<GameObject>();
				auto comp = child->NewComponent<MeshBasicRenderer>();

				comp->m_model3D		= model;
				comp->m_mesh		= model->m_meshes[node->mMeshes[i]];

				if (aiMesh->mMaterialIndex >= 0)
				{
					comp->m_texture = diffuseTextures[aiMesh->mMaterialIndex];
				}
				else
				{
					comp->m_texture = diffuseTextures.back();
				}

				compoundObj->AddChild(child);
			}

			obj->AddChild(compoundObj);
		}
		else if (node->mNumMeshes == 1)
		{
			auto aiMesh = scene->mMeshes[node->mMeshes[0]];

			auto comp = obj->NewComponent<MeshBasicRenderer>();

			comp->m_model3D		= model;
			comp->m_mesh		= model->m_meshes[node->mMeshes[0]];

			if (aiMesh->mMaterialIndex >= 0)
			{
				comp->m_texture = diffuseTextures[aiMesh->mMaterialIndex];
			}
			else
			{
				comp->m_texture = diffuseTextures.back();
			}
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
			ProcessNode(child, model, diffuseTextures, scene, node->mChildren[i]);
		}
	};

	constexpr static auto LoadMesh = [](MeshBasic* output, const aiScene* scene, aiMesh* mesh) -> void
	{
		auto graphics = Graphics::Get();

		auto numVertices = mesh->mNumVertices;
		auto aiVertices = mesh->mVertices;

		std::vector<MeshBasic::Vertex> vertices;
		vertices.reserve(mesh->mNumFaces * mesh->mFaces[0].mNumIndices);

		auto textCoord = mesh->mTextureCoords[0];

		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace& face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; j++)
			{
				auto& aiVertex = aiVertices[face.mIndices[j]];

				Vec2 uv = { 0,0 };
				if (textCoord) // does the mesh contain texture coordinates?
				{
					uv.x = textCoord[i].x;
					uv.y = textCoord[i].y;
				}

				vertices.push_back({ Vec3(aiVertex.x, aiVertex.y, aiVertex.z), uv });
			}
		}

		GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC vbDesc = {};
		vbDesc.count = vertices.size();
		vbDesc.stride = sizeof(MeshBasic::Vertex);
		output->m_vertexBuffer = graphics->CreateVertexBuffer(vbDesc);
		output->m_vertexBuffer->UpdateBuffer(vertices.data(), vbDesc.count * vbDesc.stride);

		output->m_vertexCount = vbDesc.count;

		output->m_aabb = AABox::From(vbDesc.count, 
			[&](size_t i) -> Vec3&
			{
				return vertices[i].position;
			}
		);
	};

	if (scene->HasMaterials())
	{
		auto* materials = scene->mMaterials;
		auto materialsCount = scene->mNumMaterials;

		for (size_t i = 0; i < materialsCount; i++)
		{
			auto material = materials[i];

			aiString file;
			material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), file);

			if (fs->IsFileExist(file.C_Str()))
			{
				diffuseTextures.push_back(resource::Load<Texture2D>(file.C_Str()));
			}
			else
			{
				diffuseTextures.push_back(resource::Load<Texture2D>(Texture2D::DEFAULT_FILE));
			}
		}
	}

	diffuseTextures.push_back(resource::Load<Texture2D>(Texture2D::DEFAULT_FILE));

	for (uint32_t i = 0; i < scene->mNumMeshes; i++)
	{
		auto aiMesh = scene->mMeshes[i];
		auto mesh = resource::Load<MeshBasic>(String::Format("{}|{}", path, i));

		LoadMesh(mesh, scene, aiMesh);

		mesh->m_model3DIdx = i;
		mesh->m_model3D = model3D;

		model3D->m_meshes.push_back(mesh);
	}

	ProcessNode(ret, model3D, diffuseTextures, scene, scene->mRootNode);

	return ret;
}

NAMESPACE_END