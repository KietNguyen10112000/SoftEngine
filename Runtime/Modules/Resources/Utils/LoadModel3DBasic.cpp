#include "Utils.h"

#include "../Model3DBasic.h"

#include "MainSystem/Rendering/Components/MeshBasicRenderer.h"

#include "Runtime/Runtime.h"
#include "Scene/GameObjectCache.h"

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
		output->m_vertexBuffer->UpdateBuffer(vertices.data(), vbDesc.count * vbDesc.stride, {});

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

}

NAMESPACE_END