#include "Utils.h"

#include "../Model3DBasic.h"

#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

NAMESPACE_BEGIN

namespace ResourceUtils 
{

void LoadAllMeshsForModel3DBasic(Model3DBasic* model3D, const aiScene* scene, bool ignoreBones = true)
{
	constexpr static auto LoadMesh = [](Model3DBasic::Mesh* output, const aiScene* scene, aiMesh* mesh) -> void
	{
		auto graphics = Graphics::Get();

		auto numVertices = mesh->mNumVertices;
		auto aiVertices = mesh->mVertices;

		std::vector<Model3DBasic::Vertex> vertices;
		vertices.reserve(mesh->mNumFaces * mesh->mFaces[0].mNumIndices);

		auto textCoord = mesh->mTextureCoords[0];

		Model3DBasic::Vertex vertex;
		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace& face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; j++)
			{
				auto idx = face.mIndices[j];
				auto& aiVertex = aiVertices[idx];

				vertex.position = Vec3(aiVertex.x, aiVertex.y, aiVertex.z);

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

				if (textCoord) // does the mesh contain texture coordinates?
				{
					vertex.textcoord.x = textCoord[idx].x;
					vertex.textcoord.y = textCoord[idx].y;
					//std::cout << vertex.textcoord.x << ", " << vertex.textcoord.y << "\n";
				}

				vertices.push_back(vertex);

				//vertices.back() = vertex;
			}
		}

		GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC vbDesc = {};
		vbDesc.count = vertices.size();
		vbDesc.stride = sizeof(Model3DBasic::Vertex);

		////(void)((size_t)&vertex.textcoord - (size_t)&vertex);

		////std::cout << ((size_t)&vertex.textcoord - (size_t)&vertex) << "\n";
		//for (size_t i = 0; i < sizeof(vertex); i++)
		//{
		//	std::cout << (int)*(((byte*)&vertex) + i) << ", ";
		//}

		//byte test[4] = { 0, 0, 64, 63 };
		//std::cout << *(float*)&test << "\n";
		////std::cout << sizeof(Model3DBasic::Vertex) << "\n";

		/*std::cout << "====================================\n";
		for (size_t i = 0; i < vertices.size(); i++)
		{
			std::cout << vertices[i].textcoord.x << ", " << vertices[i].textcoord.y << "\n";
		}*/

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

	size_t count = 0;
	for (uint32_t i = 0; i < scene->mNumMeshes; i++)
	{
		auto aiMesh = scene->mMeshes[i];
		if (ignoreBones || !aiMesh->HasBones())
		{
			count++;
		}
	}

	model3D->m_meshes.resize(count);

	count = 0;
	for (uint32_t i = 0; i < scene->mNumMeshes; i++)
	{
		auto aiMesh = scene->mMeshes[i];

		if (!ignoreBones && aiMesh->HasBones())
		{
			continue;
		}

		auto mesh = &model3D->m_meshes[count];

		LoadMesh(mesh, scene, aiMesh);

		mesh->m_model3DIdx = count;
		count++;
	}
}

Handle<GameObject> LoadModel3DBasic(String path, String defaultDiffusePath)
{
	auto fs = FileSystem::Get();

	if (defaultDiffusePath.empty())
	{
		defaultDiffusePath = Texture2D::DEFAULT_FILE;
	}

	auto ret = mheap::New<GameObject>();
	auto model3D = resource::Load<Model3DBasic>(path, true);

	std::vector<Resource<Texture2D>> diffuseTextures;

	std::string_view pathview(path.c_str());
	String basePath = path.SubString(0, pathview.find_last_of('/') + 1);

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fs->GetResourcesPath(path).c_str(), 
		aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded);

	constexpr static void (*ProcessNode)(GameObject*, Resource<Model3DBasic>&, std::vector<Resource<Texture2D>>&, const aiScene*, aiNode*) =
		[](GameObject* obj, Resource<Model3DBasic>& model, std::vector<Resource<Texture2D>>& diffuseTextures, const aiScene* scene, aiNode* node) {

		if (node->mNumMeshes > 1)
		{
			auto compoundObj = mheap::New<GameObject>();
			for (size_t i = 0; i < node->mNumMeshes; i++)
			{
				auto aiMesh = scene->mMeshes[node->mMeshes[i]];
				auto child = mheap::New<GameObject>();
				auto comp = child->NewComponent<MeshBasicRenderer>(false);

				comp->m_model3D		= model;
				comp->m_mesh		= &model->m_meshes[node->mMeshes[i]];

				if (aiMesh->mMaterialIndex >= 0)
				{
					comp->m_texture = diffuseTextures[aiMesh->mMaterialIndex];
				}
				else
				{
					comp->m_texture = diffuseTextures.back();
				}

				child->Name() = aiMesh->mName.C_Str();

				compoundObj->AddChild(child);				
			}

			obj->AddChild(compoundObj);
		}
		else if (node->mNumMeshes == 1)
		{
			auto aiMesh = scene->mMeshes[node->mMeshes[0]];

			auto comp = obj->NewComponent<MeshBasicRenderer>(false);

			comp->m_model3D		= model;
			comp->m_mesh		= &model->m_meshes[node->mMeshes[0]];

			if (aiMesh->mMaterialIndex >= 0)
			{
				comp->m_texture = diffuseTextures[aiMesh->mMaterialIndex];
			}
			else
			{
				comp->m_texture = diffuseTextures.back();
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
			ProcessNode(child, model, diffuseTextures, scene, node->mChildren[i]);
		}
	};

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

	LoadAllMeshsForModel3DBasic(model3D, scene);

	ProcessNode(ret, model3D, diffuseTextures, scene, scene->mRootNode);

	
	ret->Name() = path.SubString(pathview.find_last_of('/') + 1);

	return ret;
}

}

NAMESPACE_END