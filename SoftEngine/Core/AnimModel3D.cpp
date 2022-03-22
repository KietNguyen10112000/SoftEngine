#include "AnimModel3D.h"

#define ASSIMP_IMPORTER 1
 
#ifdef ASSIMP_IMPORTER
#include "ImportParser/AssimpParser.h"
#endif

#ifdef FXB_SDK_IMPORTER
#include <fbxsdk.h>
#include <Common/FbxSdkCommon.h>
#endif

#include <RenderPipeline.h>
#include <iostream>

template<typename _Vertex>
void TBNTryInitAnimableBuffer(_TBNRenderBuffer& out, AssimpParser& parser, 
	size_t maxWeightPerVertex, const aiMesh* mesh)
{
	std::vector<_Vertex> vertices;
	std::vector<uint32_t> indices;

	for (unsigned long long i = 0; i < mesh->mNumVertices; i++)
	{
		_Vertex vertex;

		Vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.pos = vector;

		if (mesh->mTangents)
		{
			vector.x = mesh->mTangents[i].x;
			vector.y = mesh->mTangents[i].y;
			vector.z = mesh->mTangents[i].z;
			vertex.tangent = vector;
		}

		if (mesh->mBitangents)
		{
			vector.x = mesh->mBitangents[i].x;
			vector.y = mesh->mBitangents[i].y;
			vector.z = mesh->mBitangents[i].z;
			vertex.bitangent = vector;
		}

		if (mesh->mNormals)
		{
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.normal = vector;
		}


		if (mesh->mTextureCoords[0])
		{
			Vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.texcoord = vec;
		}
		else
			vertex.texcoord = Vec2(0.0f, 0.0f);

		vertices.push_back(vertex);
	}

	// process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned long long j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	for (size_t i = 0; i < mesh->mNumBones; i++)
	{
		const char* name = mesh->mBones[i]->mName.C_Str();

		auto it = parser.m_nodeMap.find(name);
		assert(it != parser.m_nodeMap.end());

		auto boneIndex = parser.m_nodes[it->second].boneIndex;

		assert(boneIndex != -1);

		for (size_t j = 0; j < mesh->mBones[i]->mNumWeights; j++)
		{
			size_t vertexID = mesh->mBones[i]->mWeights[j].mVertexId;
			float weight = mesh->mBones[i]->mWeights[j].mWeight;
			/*try
			{*/
			TBNAnimModelLoader::AnimMesh::TryFill(&vertices[vertexID], boneIndex, weight, maxWeightPerVertex);
			/*}
			catch (...)
			{
				int x = 0;
			}*/
		}
	}

	out.vb = new VertexBuffer(&vertices[0], vertices.size(), sizeof(_Vertex));
	out.ib = new IndexBuffer(&indices[0], indices.size(), sizeof(uint32_t));

	using _TBNVertexBuffer = _TBNAnimModelVertexBuffer<_Vertex>;
	out.buffer = new _TBNVertexBuffer();
	auto buf = (_TBNVertexBuffer*)out.buffer;
	buf->vertices.swap(vertices);
	buf->indices.swap(indices);
}

_TBNRenderBuffer TBNInitAnimableBuffer(TBNAnimModel* init, AssimpParser& parser, const aiMesh* mesh)
{
	_TBNRenderBuffer ret;

	try
	{
		TBNTryInitAnimableBuffer<TBNAnimModelLoader::AnimMesh::WeightVertex_4>(ret, parser, 4, mesh);
		ret.rplIndex = 1;
		if (!(init->m_rpls[1])) init->m_rpls[1] = TBNAnimModelLoader::GetAnimMeshRpl(4);
		return ret;
	}
	catch (const wchar_t* e){ std::wcout << e << L"\n"; }
	
	try
	{
		TBNTryInitAnimableBuffer<TBNAnimModelLoader::AnimMesh::WeightVertex_8>(ret, parser, 8, mesh);
		ret.rplIndex = 2;
		if (!(init->m_rpls[2])) init->m_rpls[2] = TBNAnimModelLoader::GetAnimMeshRpl(8);
		return ret;
	}
	catch (const wchar_t* e) { std::wcout << e << L"\n"; }

	try
	{
		TBNTryInitAnimableBuffer<TBNAnimModelLoader::AnimMesh::WeightVertex_16>(ret, parser, 16, mesh);
		ret.rplIndex = 3;
		if (!(init->m_rpls[3])) init->m_rpls[3] = TBNAnimModelLoader::GetAnimMeshRpl(16);
		return ret;
	}
	catch (...) { assert(0 || L"Num weight per vertex too big"); }

	return ret;
}

_TBNRenderBuffer TBNInitNonAnimBuffer(TBNAnimModel* init, AssimpParser& parser, const aiMesh* mesh)
{
	using _Vertex = TBNAnimModelLoader::NonAnimMesh::Vertex;

	_TBNRenderBuffer ret;

	std::vector<_Vertex> vertices;
	std::vector<uint32_t> indices;

	for (unsigned long long i = 0; i < mesh->mNumVertices; i++)
	{
		_Vertex vertex;

		Vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.pos = vector;

		if (mesh->mTangents)
		{
			vector.x = mesh->mTangents[i].x;
			vector.y = mesh->mTangents[i].y;
			vector.z = mesh->mTangents[i].z;
			vertex.tangent = vector;
		}

		if (mesh->mBitangents)
		{
			vector.x = mesh->mBitangents[i].x;
			vector.y = mesh->mBitangents[i].y;
			vector.z = mesh->mBitangents[i].z;
			vertex.bitangent = vector;
		}

		if (mesh->mNormals)
		{
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.normal = vector;
		}

		if (mesh->mTextureCoords[0])
		{
			Vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.texcoord = vec;
		}
		else
			vertex.texcoord = Vec2(0.0f, 0.0f);

		vertices.push_back(vertex);
	}

	// process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned long long j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	ret.vb = new VertexBuffer(&vertices[0], vertices.size(), sizeof(_Vertex));
	ret.ib = new IndexBuffer(&indices[0], indices.size(), sizeof(uint32_t));

	using _TBNVertexBuffer = _TBNAnimModelVertexBuffer<decltype(vertices)::value_type>;
	ret.buffer = new _TBNVertexBuffer();
	auto buf = (_TBNVertexBuffer*)ret.buffer;
	buf->vertices.swap(vertices);
	buf->indices.swap(indices);


	void* arg[] = { (void*)&vertices, (void*)&indices };
	using VContainerType = std::remove_reference<decltype(vertices)>::type;
	using IContainerType = std::remove_reference<decltype(indices)>::type;
	auto lambda = [](size_t i, void** arg) -> Vec3&
	{
		VContainerType& vertices = *(VContainerType*)arg[0];
		IContainerType& indices = *(IContainerType*)arg[1];

		return vertices[indices[i]].pos;
	};
	buf->nonAnimMeshAABB = AABB::From<lambda>(indices.size(), arg);


	ret.rplIndex = 0;
	if (!(init->m_rpls[0])) init->m_rpls[0] = TBNAnimModelLoader::GetNonAnimMeshRpl();

	return ret;
}

void TBNProcessMaterials(const aiScene* scene, aiMesh* mesh, void** argv, int argc)
{
	if (scene->HasMaterials() && argc >= 1)
	{
		std::vector<std::wstring>* list = (std::vector<std::wstring>*)argv[0];

		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		//get pbr material
		aiString file;
		material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), file);
		list->push_back(StringToWString(file.C_Str()));
		file.Clear();
		material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), file);
		list->push_back(StringToWString(file.C_Str()));
		file.Clear();
		material->Get(AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0), file);
		list->push_back(StringToWString(file.C_Str()));
		file.Clear();
		material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0), file);
		list->push_back(StringToWString(file.C_Str()));
		file.Clear();
		material->Get(AI_MATKEY_TEXTURE(aiTextureType_AMBIENT_OCCLUSION, 0), file);
		list->push_back(StringToWString(file.C_Str()));
		file.Clear();
	}
}

void TBNProcessMeshs(AssimpParser& parser, TBNAnimModel* init, const aiScene* scene, void** argv = 0, int argc = 0)
{
	init->m_renderBuf.resize(scene->mNumMeshes);
	for (size_t i = 0; i < parser.m_nodes.size(); i++)
	{
		auto& node = parser.m_nodes[i];
		if (node.numMeshs == 0) continue;

		for (size_t j = 0; j < node.numMeshs; j++)
		{
			auto& mesh = scene->mMeshes[node.meshsIndex[j]];

			auto& cur = init->m_renderBuf[node.meshsIndex[j]];

			if (mesh->mNumBones != 0)
			{
				cur = TBNInitAnimableBuffer(init, parser, mesh);
			}
			else
			{
				cur = TBNInitNonAnimBuffer(init, parser, mesh);
			}
			cur.localTransform = node.localTransform;

			if (argc > 0) TBNProcessMaterials(scene, mesh, argv, argc);
		}
	}
}

template<typename _RenderBuffer>
void ProcessAnimations(AssimpParser& parser, AnimModel<_RenderBuffer>* init, const aiScene* scene)
{
	auto& store = init->m_animations;
	for (auto& anim : parser.m_animations)
	{
		Animation animation;
		animation.ticksPerSecond = anim.ticksPerSecond;
		animation.tickDuration = anim.duration;
		animation.name = anim.aiAnim->mName.C_Str();
		
		for (auto& animNode : anim.channels)
		{
			KeyFrames keyframes;
			keyframes.nodeId = animNode.nodeId;

			ScalingKeyFrame sKey;
			for (size_t i = 0; i < animNode.nodeAnim->mNumScalingKeys; i++)
			{
				auto& scaling = animNode.nodeAnim->mScalingKeys[i];
				sKey.value.x = scaling.mValue.x;
				sKey.value.y = scaling.mValue.y;
				sKey.value.z = scaling.mValue.z;
				sKey.time = scaling.mTime;
				keyframes.scaling.push_back(sKey);
			}

			RotaionKeyFrame rKey;
			for (size_t i = 0; i < animNode.nodeAnim->mNumRotationKeys; i++)
			{
				auto& rotation = animNode.nodeAnim->mRotationKeys[i];
				rKey.value.x = rotation.mValue.x;
				rKey.value.y = rotation.mValue.y;
				rKey.value.z = rotation.mValue.z;
				rKey.value.w = rotation.mValue.w;
				rKey.time = rotation.mTime;
				keyframes.rotation.push_back(rKey);
			}

			TranslationKeyFrame tKey;
			for (size_t i = 0; i < animNode.nodeAnim->mNumPositionKeys; i++)
			{
				auto& pos = animNode.nodeAnim->mPositionKeys[i];
				tKey.value.x = pos.mValue.x;
				tKey.value.y = pos.mValue.y;
				tKey.value.z = pos.mValue.z;
				tKey.time = pos.mTime;
				keyframes.translation.push_back(tKey);
			}

			animation.channels.push_back(keyframes);
		}
		store.push_back(animation);
	}
}

template<typename _RenderBuffer>
void ProcessNodes(AssimpParser& parser, AnimModel<_RenderBuffer>* init, const aiScene* scene)
{
	init->m_nodes.resize(parser.m_nodes.size());

	for (size_t i = 0; i < parser.m_nodes.size(); i++)
	{
		auto& pNode = parser.m_nodes[i];
		auto& iNode = init->m_nodes[i];

		iNode.boneIndex = pNode.boneIndex;
		iNode.numMeshs = pNode.numMeshs;
		iNode.parentIndex = pNode.parentIndex;
		iNode.meshsIndex = new uint32_t[iNode.numMeshs];
		iNode.localTransform = pNode.localTransform;
		::memcpy(iNode.meshsIndex, pNode.meshsIndex, iNode.numMeshs * sizeof(uint32_t));
	}
}

void TBNAnimModelLoader::Load(const std::wstring& path, Mat4x4& preTransform, TBNAnimModel* init, void** argv, int argc)
{
#ifdef FXB_SDK_IMPORTER
    FbxManager* lSdkManager = NULL;
    FbxScene* lScene = NULL;
    InitializeSdkObjects(lSdkManager, lScene);
    if (!LoadScene(lSdkManager, lScene, WStringToString(path).c_str()))
    {
        Throw("FBX Importer Error");
    }
    FBXImporter::Traversal(lSdkManager, lScene, init, preTransform, FBXImporter::TBNAnimModel_ProcessMesh);
    init->m_rpls[0] = GetNonAnimMeshRpl();
    DestroySdkObjects(lSdkManager, 1);
#endif

#ifdef ASSIMP_IMPORTER
	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_CAMERAS | aiComponent_LIGHTS | aiComponent_TEXTURES);
	//importer.SetPropertyInteger(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, 0);

	//importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	const aiScene* scene = importer.ReadFile(WStringToString(path).c_str(), aiProcess_ConvertToLeftHanded
		| aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_RemoveComponent);


	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		Throw(L"ERROR::ASSIMP model loading");
	}

	if (scene->mNumAnimations == 0)
	{
		Throw(L"Model was not animated");
	}

	AssimpParser parser;
	parser.m_preTransform = preTransform;
	parser.Vectorize(scene);

	init->m_bonesOffset = parser.m_boneOffsets;
	//init->m_bonesParentId = parser.m_parentBoneId;

	if(argc >= 2) argv[0] = argv[1];
	TBNProcessMeshs(parser, init, scene, argv, argc - 1);

	ProcessNodes(parser, init, scene);

	ProcessAnimations(parser, init, scene);

	importer.FreeScene();
#endif

}

void TBNAnimModelLoader::FreeTempBuffer(_TBNRenderBuffer* buffer)
{
	if (!(buffer->buffer)) return;

	using W0 = TBNAnimModelLoader::NonAnimMesh::Vertex;
	using W4 = TBNAnimModelLoader::AnimMesh::WeightVertex_4;
	using W8 = TBNAnimModelLoader::AnimMesh::WeightVertex_8;
	switch (buffer->rplIndex)
	{
	case 0:
		Free((_TBNAnimModelVertexBuffer<W0>**) & buffer->buffer);
		break;
	case 1:
		Free((_TBNAnimModelVertexBuffer<W4>**) & buffer->buffer);
		break;
	case 2:
		Free((_TBNAnimModelVertexBuffer<W8>**) & buffer->buffer);
		break;
	case 3:
		assert(0);
		break;
	default:
		break;
	}
}

void TBNAnimModelLoader::Free(_TBNRenderBuffer* buffer)
{
	delete buffer->vb;
	delete buffer->ib;

	FreeTempBuffer(buffer);
}

RenderPipeline* TBNAnimModelLoader::GetNonAnimMeshRpl()
{
	return RenderPipelineManager::Get(
        R"(
		struct 
		{
			Vec3 position; POSITION, PER_VERTEX #
			Vec2 textCoord; TEXTCOORD, PER_VERTEX #
			Vec3 normal; NORMAL, PER_VERTEX #
			Vec3 tangent; TANGENT, PER_VERTEX #
			Vec3 bitangent; BITANGENT, PER_VERTEX #
		}
		)",
        L"AnimModel3D/VS_NonAnim_TBN_Mesh",
        L"AnimModel3D/PS_TBN_Mesh"
        //L"Model3D/PBR/PSPBRModel"
    );
}

RenderPipeline* TBNAnimModelLoader::GetAnimMeshRpl(size_t maxWeightPerVertex)
{
	switch (maxWeightPerVertex)
	{
	case TBNAnimModelLoader::AnimMesh::FOUR:
		return RenderPipelineManager::Get(
			R"(
			struct 
			{
				Vec3 position; POSITION, PER_VERTEX #
				Vec2 textCoord; TEXTCOORD, PER_VERTEX #
				Vec3 normal; NORMAL, PER_VERTEX #
				Vec3 tangent; TANGENT, PER_VERTEX #
				Vec3 bitangent; BITANGENT, PER_VERTEX #

				Vec4us bone; BONE1, PER_VERTEX #
				Vec4 weight; WEIGHT1, PER_VERTEX #
			}
			)",
				L"AnimModel3D/VS_4_Weight_TBN_Mesh",
				L"AnimModel3D/PS_TBN_Mesh"
			);
		break;
	case TBNAnimModelLoader::AnimMesh::EIGHT:
		return RenderPipelineManager::Get(
			R"(
			struct 
			{
				Vec3 position; POSITION, PER_VERTEX #
				Vec2 textCoord; TEXTCOORD, PER_VERTEX #
				Vec3 normal; NORMAL, PER_VERTEX #
				Vec3 tangent; TANGENT, PER_VERTEX #
				Vec3 bitangent; BITANGENT, PER_VERTEX #

				Vec4us bone1; BONE1, PER_VERTEX #
				Vec4us bone2; BONE2, PER_VERTEX #
				Vec4 weight1; WEIGHT1, PER_VERTEX #
				Vec4 weight2; WEIGHT2, PER_VERTEX #
			}
			)",
			L"AnimModel3D/VS_8_Weight_TBN_Mesh",
			L"AnimModel3D/PS_TBN_Mesh"
			);
		break;
	case TBNAnimModelLoader::AnimMesh::SIXTEEN:
		assert(0);
		break;
	default:
		break;
	}
	return nullptr;
}


template <typename T, size_t WEIGHT_COUNT>
AABB TBNAnimMesh_CalculateAABB(_TBNAnimModelVertexBuffer<T>* buffer,
	std::vector<Mat4x4*>& meshLocalTransform, std::vector<Mat4x4>& bones)
{
	//do as gpu transform =)))
	auto& indices = buffer->indices;
	auto& vertices = buffer->vertices;

	Mat4x4 boneTransform;
	//Mat4x4 temp;

	Vec4 pos;

	std::vector<Vec3> positions;
	positions.reserve(indices.size());

	for (auto& id : indices)
	{
		auto& currentVertex = vertices[id];

		pos = Vec4(currentVertex.pos, 1.0f);

		if constexpr (WEIGHT_COUNT >= 4)
		{
			boneTransform  = bones[currentVertex.boneID1[0]] * currentVertex.weight1[0];
			boneTransform += bones[currentVertex.boneID1[1]] * currentVertex.weight1[1];
			boneTransform += bones[currentVertex.boneID1[2]] * currentVertex.weight1[2];
			boneTransform += bones[currentVertex.boneID1[3]] * currentVertex.weight1[3];
		}
		if constexpr (WEIGHT_COUNT >= 8)
		{
			boneTransform += bones[currentVertex.boneID2[0]] * currentVertex.weight2[0];
			boneTransform += bones[currentVertex.boneID2[1]] * currentVertex.weight2[1];
			boneTransform += bones[currentVertex.boneID2[2]] * currentVertex.weight2[2];
			boneTransform += bones[currentVertex.boneID2[3]] * currentVertex.weight2[3];
		}
		if constexpr (WEIGHT_COUNT >= 16)
		{
			boneTransform += bones[currentVertex.boneID3[0]] * currentVertex.weight3[0];
			boneTransform += bones[currentVertex.boneID3[1]] * currentVertex.weight3[1];
			boneTransform += bones[currentVertex.boneID3[2]] * currentVertex.weight3[2];
			boneTransform += bones[currentVertex.boneID3[3]] * currentVertex.weight3[3];

			boneTransform += bones[currentVertex.boneID4[0]] * currentVertex.weight4[0];
			boneTransform += bones[currentVertex.boneID4[1]] * currentVertex.weight4[1];
			boneTransform += bones[currentVertex.boneID4[2]] * currentVertex.weight4[2];
			boneTransform += bones[currentVertex.boneID4[3]] * currentVertex.weight4[3];
		}

		pos = pos * boneTransform;
		pos = pos / pos.w;

		positions.push_back(ConvertVector(pos));
	}

	AABB ret;
	ret.FromPoints(positions.data(), positions.size());

	return ret;
}

//template <typename T, size_t WEIGHT_COUNT>
//AABB TBNNonAnimMesh_CalculateAABB(_TBNAnimModelVertexBuffer<T>* buffer,
//	std::vector<Mat4x4*>& meshLocalTransform, std::vector<Mat4x4>& bones)
//{
//	AABB ret;
//	//ret.FromPoints(positions.data(), positions.size());
//
//	return ret;
//}

void AnimModelAABBCalculator::TBNAnimModel_CalculateAABB(TBNAnimModel* model, Animation* animation, 
	size_t id, float t, std::vector<Mat4x4*>& meshLocalTransform, std::vector<Mat4x4>& bones)
{
	using W0 = TBNAnimModelLoader::NonAnimMesh::Vertex;
	using W4 = TBNAnimModelLoader::AnimMesh::WeightVertex_4;
	using W8 = TBNAnimModelLoader::AnimMesh::WeightVertex_8;

	auto& meshAABBs = animation->meshAABBs;

	const auto numMesh = model->m_renderBuf.size();
	for (size_t i = 0; i < numMesh; i++)
	{
		auto& rdb = model->m_renderBuf[i];

		AABB ret;
		
		switch (rdb.rplIndex)
		{
		case 0:
			ret = ((_TBNAnimModelVertexBuffer<W0>*)rdb.buffer)->nonAnimMeshAABB;
			break;
		case 1:
			ret = TBNAnimMesh_CalculateAABB<W4, 4>((_TBNAnimModelVertexBuffer<W4>*)rdb.buffer, meshLocalTransform, bones);
			break;
		case 2:
			ret = TBNAnimMesh_CalculateAABB<W8, 8>((_TBNAnimModelVertexBuffer<W8>*)rdb.buffer, meshLocalTransform, bones);
			break;
		case 3:
			assert(0);
			break;
		default:
			break;
		}

		if (meshLocalTransform[i]) ret.Transform(*(meshLocalTransform[i]));

		meshAABBs[i].aabb.push_back({ ret, t });
	}
}
