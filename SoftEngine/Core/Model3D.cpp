#include "Model3D.h"

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <RenderPipeline.h>

class ModelLoaderHelperFunc
{
public:
	inline static void ConvertAssimp4x4ToMat4x4(Mat4x4* dest, aiMatrix4x4* src);
	inline static void BasicModel_ProcessNode(aiNode* node, const aiScene* scene, 
		BasicModel* init, Mat4x4* preTransform, Mat4x4 currentLocalTransform, 
		std::vector<BasicModel::Vertex>& vertices, std::vector<unsigned int>& indices);

	//ProcessMeshFnc procotype : 
	//void(*)(T* init, Mat4x4* localTransform, aiMesh* mesh, std::vector& verticesBuffer,  std::vector& indicesBuffer);
	template <typename T, typename T_Vertex, typename _Fnc, typename ... _Args>
	inline static void ProcessNode(aiNode* node, const aiScene* scene, T* init,
		Mat4x4* preTransform, Mat4x4 currentLocalTransform,
		std::vector<T_Vertex>& vertices, std::vector<unsigned int>& indices,
		_Fnc ProcessMeshFnc, _Args&& ...args
	);

	inline static void TBNModel_ProcessNode(aiNode* node, const aiScene* scene,
		TBNModel* init, Mat4x4* preTransform, Mat4x4 currentLocalTransform,
		std::vector<TBNModel::Vertex>& vertices, std::vector<unsigned int>& indices);

	inline static void RawTBNModel_ProcessNode(aiNode* node, const aiScene* scene,
		RawTBNModel* init, Mat4x4* preTransform, Mat4x4 currentLocalTransform,
		std::vector<RawTBNModel::Vertex>& vertices, std::vector<unsigned int>& indices);


};


BasicModel::BasicModel(const std::wstring& path, uint32_t numArg, void** args) : IResource(path)
{
#ifdef BASIC_MODEL_UNIQUE_SHADER_VAR
	if (!localTransformShaderVar)
	{
		Mat4x4 temp;
		localTransformShaderVar = new ShaderVar(&temp, sizeof(Mat4x4));

		BasicModel::MeshMaterial temp1;
		materialShaderVar = new ShaderVar(&temp1, sizeof(BasicModel::MeshMaterial));
	}
	count++;
#endif

	Mat4x4 preTransform;
	bool flag = false;

	if (numArg != 0)
	{
		preTransform = *(Mat4x4*)args[0];

		Mat4x4 iden;
		if (::memcmp(&preTransform, &iden, sizeof(Mat4x4)) != 0)
		{
			flag = true;
		}

	}

	Assimp::Importer importer;

	auto cpath = WStringToString(path);

	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_CAMERAS | aiComponent_LIGHTS | aiComponent_TEXTURES);

	const aiScene* scene = importer.ReadFile(cpath.c_str(), aiProcess_ConvertToLeftHanded
		| aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_RemoveComponent);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << '\n';
		return;
	}

#ifdef STATIC_MODEL3D_THROW_IF_ANIMATED
	if (scene->mNumAnimations != 0)
	{
		Throw(L"Model was animated.");
	}
#endif // STATIC_MODEL3D_THROW_IF_ANIMATED

	std::vector<BasicModel::Vertex> vertices;
	std::vector<unsigned int> indices;
	Mat4x4 currentLocalTransform;

	ModelLoaderHelperFunc::BasicModel_ProcessNode(scene->mRootNode, scene, this, 
		flag ? &preTransform : nullptr, currentLocalTransform, vertices, indices);

	importer.FreeScene();

}

BasicModel::~BasicModel()
{
#ifdef BASIC_MODEL_UNIQUE_SHADER_VAR
	count--;

	if (count == 0)
	{
		delete localTransformShaderVar;
		delete materialShaderVar;
	}

#endif

	for (auto& mesh : m_meshs)
	{
		delete mesh.ib;
		delete mesh.vb;
#ifndef BASIC_MODEL_UNIQUE_SHADER_VAR
		delete mesh.svar;
		delete mesh.materialSvar;
#endif
	}
}

void BasicModel::Render(IRenderer* renderer, RenderPipeline* rpl)
{
#ifdef BASIC_MODEL_UNIQUE_SHADER_VAR
	rpl->PSSetVar(materialShaderVar, MESH_PS_MATERIAL_LOCATION);
	rpl->VSSetVar(localTransformShaderVar, MESH_LOCAL_TRANSFORM_LOCATION);
	for (auto& mesh : m_meshs)
	{
		localTransformShaderVar->Update(&mesh.localTransform, sizeof(Mat4x4));
		materialShaderVar->Update(&mesh.material, sizeof(MeshMaterial));
		renderer->Render(rpl, mesh.vb, mesh.ib);
	}

#else

	for (auto& mesh : m_meshs)
	{
		rpl->VSSetVar(mesh.svar, MESH_LOCAL_TRANSFORM_LOCATION);
		rpl->PSSetVar(mesh.materialSvar, MESH_PS_MATERIAL_LOCATION);
		renderer->Render(rpl, mesh.vb, mesh.ib);
	}

#endif
	
}

inline void ModelLoaderHelperFunc::ConvertAssimp4x4ToMat4x4(Mat4x4* dest, aiMatrix4x4* src)
{
	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			dest->m[i][j] = (*src)[j][i];
		}
	}
}

inline void ModelLoaderHelperFunc::BasicModel_ProcessNode(aiNode* node, const aiScene* scene, 
	BasicModel* init, Mat4x4* preTransform, Mat4x4 currentLocalTransform, 
	std::vector<BasicModel::Vertex>& vertices, std::vector<unsigned int>& indices)
{
	auto ProcessMesh = [&](aiMesh* mesh, const aiScene* scene, Mat4x4* localTransform) -> BasicModel::Mesh
	{
		BasicModel::Mesh ret;

		vertices.resize(mesh->mNumVertices);

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			BasicModel::Vertex& vertex = vertices[i];
			// process vertex positions, normals and texture coordinates
			Vec3 vector;
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.position = vector;

			if (mesh->mNormals)
			{
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.normal = vector;
			}

			if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
			{
				Vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.textCoord = vec;
			}
			else
				vertex.textCoord = Vec2(0.0f, 0.0f);
		}

		// process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		// process material
		BasicModel::MeshMaterial meshMaterial;
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			aiColor3D color(0.f, 0.f, 0.f);
			float tempFloat = 0.0f;

			if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color))
			{
				meshMaterial.diffuse = { color[0], color[1], color[2] };
			}

			if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, color))
			{
				meshMaterial.ambient = { color[0], color[1], color[2] };
			}

			if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, color))
			{
				meshMaterial.specular = color[0];
			}

			if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, tempFloat))
			{
				meshMaterial.shininess = tempFloat;
			}

		}

#ifndef KEEP_MESH_DATA
#ifdef	BASIC_MODEL_KEEP_MESH_MATERIAL
		ret.material = meshMaterial;
#endif
#endif

#ifdef KEEP_MESH_DATA
		ret.vertices = vertices;
		ret.indices = indices;
		ret.material = meshMaterial;
#endif

#ifdef KEEP_MESH_NAME
		ret.name = StringToWString(std::string(mesh->mName.C_Str()));
#endif

		ret.vb = new VertexBuffer(&vertices[0], vertices.size(), sizeof(BasicModel::Vertex));
		ret.ib = new IndexBuffer(&indices[0], indices.size(), sizeof(unsigned int));

		void* arg[] = { (void*)&vertices, (void*)&indices };

		using VContainerType = std::remove_reference<decltype(vertices)>::type;
		using IContainerType = std::remove_reference<decltype(indices)>::type;
		auto lambda = [](size_t i, void** arg) -> Vec3&
		{
			VContainerType& vertices = *(VContainerType*)arg[0];
			IContainerType& indices = *(IContainerType*)arg[1];

			return vertices[indices[i]].position;
		};

		ret.aabb = AABB::From<lambda>(indices.size(), arg);
		if (localTransform)
			ret.aabb.Transform(*localTransform);

#ifndef BASIC_MODEL_UNIQUE_SHADER_VAR
		ret.materialSvar = new ShaderVar(&meshMaterial, sizeof(BasicModel::MeshMaterial));
#else
		ret.material = meshMaterial;
#endif
		return ret;
	};

	Mat4x4 localTransform;

	ConvertAssimp4x4ToMat4x4(&(localTransform), &(node->mTransformation));

	currentLocalTransform *= localTransform;

	// process all the node's meshes (if any)
	for (unsigned long long i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

		vertices.clear();
		indices.clear();

		if (preTransform) localTransform = currentLocalTransform * (*preTransform);

		BasicModel::Mesh temp = ProcessMesh(mesh, scene, &localTransform);

#ifndef BASIC_MODEL_UNIQUE_SHADER_VAR
		temp.svar = new ShaderVar(&localTransform, sizeof(Mat4x4));
#else
		temp.localTransform = localTransform;
#endif

		init->m_meshs.push_back(temp);
	}
	// then do the same for each of its children
	for (unsigned long long i = 0; i < node->mNumChildren; i++)
	{
		BasicModel_ProcessNode(node->mChildren[i], scene, init, preTransform, 
			currentLocalTransform, vertices, indices);
	}
}


TBNModel::TBNModel(const std::wstring& path, uint32_t numArg, void** args) : IResource(path)
{
#ifdef TBN_MODEL_UNIQUE_SHADER_VAR
	if (!localTransformShaderVar)
	{
		Mat4x4 temp;
		localTransformShaderVar = new ShaderVar(&temp, sizeof(Mat4x4));
	}
	count++;
#endif

	Mat4x4 preTransform;
	bool flag = false;

	if (numArg != 0)
	{
		preTransform = *(Mat4x4*)args[0];
		Mat4x4 iden;
		if (::memcmp(&preTransform, &iden, sizeof(Mat4x4)) != 0)
		{
			flag = true;
		}
	}

	Assimp::Importer importer;

	auto cpath = WStringToString(path);

	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_CAMERAS | aiComponent_LIGHTS | aiComponent_TEXTURES);

	const aiScene* scene = importer.ReadFile(cpath.c_str(), aiProcess_ConvertToLeftHanded
		| aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_RemoveComponent);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << '\n';
		return;
	}

#ifdef STATIC_MODEL3D_THROW_IF_ANIMATED
	if (scene->mNumAnimations != 0)
	{
		Throw(L"Model was animated.");
	}
#endif // STATIC_MODEL3D_THROW_IF_ANIMATED

	std::vector<TBNModel::Vertex> vertices;
	std::vector<unsigned int> indices;
	Mat4x4 currentLocalTransform;

	//scene = importer.ApplyPostProcessing(aiProcess_GenNormals);

	ModelLoaderHelperFunc::TBNModel_ProcessNode(scene->mRootNode, scene, this, 
		flag ? &preTransform : nullptr, currentLocalTransform, vertices, indices);

	importer.FreeScene();

}

TBNModel::~TBNModel()
{
#ifdef TBN_MODEL_UNIQUE_SHADER_VAR
	count--;

	if (count == 0)
	{
		delete localTransformShaderVar;
		delete materialShaderVar;
	}

#endif

	for (auto& mesh : m_meshs)
	{
		delete mesh.ib;
		delete mesh.vb;
#ifndef TBN_MODEL_UNIQUE_SHADER_VAR
		delete mesh.svar;
		delete mesh.materialSvar;
#endif
	}
}

void TBNModel::Render(IRenderer* renderer, RenderPipeline* rpl)
{
#ifdef TBN_MODEL_UNIQUE_SHADER_VAR
	rpl->PSSetVar(materialShaderVar, MESH_PS_MATERIAL_LOCATION);
	rpl->VSSetVar(localTransformShaderVar, MESH_LOCAL_TRANSFORM_LOCATION);
	for (auto& mesh : m_meshs)
	{
		localTransformShaderVar->Update(&mesh.localTransform, sizeof(Mat4x4));
		materialShaderVar->Update(&mesh.material, sizeof(MeshMaterial));
		renderer->Render(rpl, mesh.vb, mesh.ib);
	}

#else

	for (auto& mesh : m_meshs)
	{
		rpl->VSSetVar(mesh.svar, MESH_LOCAL_TRANSFORM_LOCATION);
		rpl->PSSetVar(mesh.materialSvar, MESH_PS_MATERIAL_LOCATION);
		renderer->Render(rpl, mesh.vb, mesh.ib);
	}

#endif
}

inline void ModelLoaderHelperFunc::TBNModel_ProcessNode(aiNode* node, const aiScene* scene,
	TBNModel* init, Mat4x4* preTransform, Mat4x4 currentLocalTransform,
	std::vector<TBNModel::Vertex>& vertices, std::vector<unsigned int>& indices)
{
	auto ProcessMesh = [&](
		TBNModel* init, Mat4x4* localTransform, 
		aiMesh* mesh, 
		std::vector<TBNModel::Vertex>& vertices, std::vector<unsigned int>& indices) -> void
	{
		TBNModel::Mesh ret;

		vertices.resize(mesh->mNumVertices);

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			TBNModel::Vertex& vertex = vertices[i];
			// process vertex positions, normals and texture coordinates
			Vec3 vector;
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.position = vector;

			if (mesh->mTangents)
			{
				vertex.tangent.x = mesh->mTangents[i].x;
				vertex.tangent.y = mesh->mTangents[i].y;
				vertex.tangent.z = mesh->mTangents[i].z;
			}

			if (mesh->mBitangents)
			{
				vertex.bitangent.x = mesh->mBitangents[i].x;
				vertex.bitangent.y = mesh->mBitangents[i].y;
				vertex.bitangent.z = mesh->mBitangents[i].z;
			}

			if (mesh->mNormals)
			{
				vertex.normal.x = mesh->mNormals[i].x;
				vertex.normal.y = mesh->mNormals[i].y;
				vertex.normal.z = mesh->mNormals[i].z;
			}

			if (mesh->mTextureCoords[0])
			{
				Vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.textCoord = vec;
			}
			else
				vertex.textCoord = Vec2(0.0f, 0.0f);

		}

		// process material
		TBNModel::MeshMaterial meshMaterial;
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			aiColor3D color(0.f, 0.f, 0.f);
			float tempFloat = 0.0f;

			if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color))
			{
				meshMaterial.diffuse = { color[0], color[1], color[2] };
			}

			if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, color))
			{
				meshMaterial.ambient = { color[0], color[1], color[2] };
			}

			if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, color))
			{
				meshMaterial.specular = color[0];
			}

			if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, tempFloat))
			{
				meshMaterial.shininess = tempFloat;
			}

		}

		// process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

#ifdef KEEP_MESH_DATA
		ret.vertices = vertices;
		ret.indices = indices;
		ret.material = meshMaterial;
		ret.localTransform = localTransform;
#endif

#ifdef KEEP_MESH_NAME
		ret.name = StringToWString(std::string(mesh->mName.C_Str()));
#endif

		ret.vb = new VertexBuffer(&vertices[0], vertices.size(), sizeof(TBNModel::Vertex));
		ret.ib = new IndexBuffer(&indices[0], indices.size(), sizeof(unsigned int));

		void* arg[] = { (void*)&vertices, (void*)&indices };
		using VContainerType = std::remove_reference<decltype(vertices)>::type;
		using IContainerType = std::remove_reference<decltype(indices)>::type;
		auto lambda = [](size_t i, void** arg) -> Vec3&
		{
			VContainerType& vertices = *(VContainerType*)arg[0];
			IContainerType& indices = *(IContainerType*)arg[1];

			return vertices[indices[i]].position;
		};
		ret.aabb = AABB::From<lambda>(indices.size(), arg);
		if (localTransform)
			ret.aabb.Transform(*localTransform);

#ifndef TBN_MODEL_UNIQUE_SHADER_VAR
		ret.svar = new ShaderVar(localTransform, sizeof(Mat4x4));
		ret.materialSvar = new ShaderVar(&meshMaterial, sizeof(TBNModel::MeshMaterial));
#else
		ret.localTransform = localTransform;
		ret.material = meshMaterial;
#endif

#ifndef KEEP_MESH_DATA
#ifdef	TBN_MODEL_KEEP_MESH_MATERIAL
		ret.material = meshMaterial;
#endif
#endif

		init->m_meshs.push_back(ret);
	};

	ProcessNode(node, scene, init, preTransform, currentLocalTransform, vertices, indices, ProcessMesh);
}


//ProcessMeshFnc procotype : 
//void(*)(T* init, Mat4x4* localTransform, aiMesh* mesh, std::vector& verticesBuffer,  std::vector& indicesBuffer);
template<typename T, typename T_Vertex, typename _Fnc, typename ... _Args>
inline void ModelLoaderHelperFunc::ProcessNode(aiNode* node, const aiScene* scene, 
	T* init, Mat4x4* preTransform, Mat4x4 currentLocalTransform, 
	std::vector<T_Vertex>& vertices, std::vector<unsigned int>& indices,
	_Fnc ProcessMeshFnc, _Args&& ...args)
{
	Mat4x4 localTransform;

	ConvertAssimp4x4ToMat4x4(&(localTransform), &(node->mTransformation));

	currentLocalTransform *= localTransform;

	// process all the node's meshes (if any)
	for (unsigned long long i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

		vertices.clear();
		indices.clear();

		if (preTransform) localTransform = currentLocalTransform * (*preTransform);

		ProcessMeshFnc(init, &localTransform, mesh, vertices, indices, std::forward<_Args>(args) ...);

	}
	// then do the same for each of its children
	for (unsigned long long i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, init, preTransform,
			currentLocalTransform, vertices, indices, ProcessMeshFnc, std::forward<_Args>(args) ...);
	}
}


inline void ModelLoaderHelperFunc::RawTBNModel_ProcessNode(aiNode* node, const aiScene* scene,
	RawTBNModel* init, Mat4x4* preTransform, Mat4x4 currentLocalTransform,
	std::vector<RawTBNModel::Vertex>& vertices, std::vector<unsigned int>& indices)
{
	auto ProcessMesh = [&](
		RawTBNModel* init, Mat4x4* localTransform,
		aiMesh* mesh,
		std::vector<RawTBNModel::Vertex>& vertices, std::vector<unsigned int>& indices) -> void
	{
		RawTBNModel::Mesh ret;

		vertices.resize(mesh->mNumVertices);

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			RawTBNModel::Vertex& vertex = vertices[i];
			// process vertex positions, normals and texture coordinates
			Vec3 vector;
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.position = vector;

			if (mesh->mTangents)
			{
				vertex.tangent.x = mesh->mTangents[i].x;
				vertex.tangent.y = mesh->mTangents[i].y;
				vertex.tangent.z = mesh->mTangents[i].z;
			}

			if (mesh->mBitangents)
			{
				vertex.bitangent.x = mesh->mBitangents[i].x;
				vertex.bitangent.y = mesh->mBitangents[i].y;
				vertex.bitangent.z = mesh->mBitangents[i].z;
			}

			if (mesh->mNormals)
			{
				vertex.normal.x = mesh->mNormals[i].x;
				vertex.normal.y = mesh->mNormals[i].y;
				vertex.normal.z = mesh->mNormals[i].z;
			}

			if (mesh->mTextureCoords[0])
			{
				Vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.textCoord = vec;
			}
			else
				vertex.textCoord = Vec2(0.0f, 0.0f);

		}

		// process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		ret.vb = new VertexBuffer(&vertices[0], vertices.size(), sizeof(TBNModel::Vertex));
		ret.ib = new IndexBuffer(&indices[0], indices.size(), sizeof(unsigned int));

		void* arg[] = { (void*)&vertices, (void*)&indices };
		using VContainerType = std::remove_reference<decltype(vertices)>::type;
		using IContainerType = std::remove_reference<decltype(indices)>::type;
		auto lambda = [](size_t i, void** arg) -> Vec3&
		{
			VContainerType& vertices = *(VContainerType*)arg[0];
			IContainerType& indices = *(IContainerType*)arg[1];

			return vertices[indices[i]].position;
		};
		ret.aabb = AABB::From<lambda>(indices.size(), arg);

		if (localTransform)
			ret.aabb.Transform(*localTransform);

		ret.svar = new ShaderVar(localTransform, sizeof(Mat4x4));

		init->m_meshs.push_back(ret);
	};

	ProcessNode(node, scene, init, preTransform, currentLocalTransform, vertices, indices, ProcessMesh);
}

RawTBNModel::RawTBNModel(const std::wstring& path, uint32_t numArg, void** args) : IResource(path)
{
	Mat4x4 preTransform;
	bool flag = false;

	if (numArg != 0)
	{
		preTransform = *(Mat4x4*)args[0];
		Mat4x4 iden;
		if (::memcmp(&preTransform, &iden, sizeof(Mat4x4)) != 0)
		{
			flag = true;
		}
	}

	Assimp::Importer importer;

	auto cpath = WStringToString(path);

	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_CAMERAS | aiComponent_LIGHTS | aiComponent_TEXTURES);

	const aiScene* scene = importer.ReadFile(cpath.c_str(), aiProcess_ConvertToLeftHanded
		| aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_RemoveComponent);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << '\n';
		return;
	}

	std::vector<RawTBNModel::Vertex> vertices;
	std::vector<unsigned int> indices;
	Mat4x4 currentLocalTransform;

	ModelLoaderHelperFunc::RawTBNModel_ProcessNode(scene->mRootNode, scene, this,
		flag ? &preTransform : nullptr, currentLocalTransform, vertices, indices);

	importer.FreeScene();
}

RawTBNModel::~RawTBNModel()
{
	for (auto& mesh : m_meshs)
	{
		delete mesh.ib;
		delete mesh.vb;
		delete mesh.svar;
	}
}

void RawTBNModel::Render(IRenderer* renderer, RenderPipeline* rpl)
{
	for (auto& mesh : m_meshs)
	{
		rpl->VSSetVar(mesh.svar, MESH_LOCAL_TRANSFORM_LOCATION);
		renderer->Render(rpl, mesh.vb, mesh.ib);
	}
}
