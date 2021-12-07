#include "AssimpParser.h"

#include <cassert>

void MapBones(AssimpParser* parser, aiNode* node, const aiScene* scene)
{
	for (size_t i = 0; i < node->mNumMeshes; i++)
	{
		auto meshId = node->mMeshes[i];
		auto& mesh = scene->mMeshes[i];
		for (size_t j = 0; j < mesh->mNumBones; j++)
		{
			//assimp bone is just a node in hierarchy 
			auto& map = parser->m_nodeMap;
			auto it = map.find(mesh->mBones[j]->mName.C_Str());

			assert(it != map.end());

			auto nodeId = it->second;

			if (parser->m_nodes[nodeId].boneIndex == -1)
			{
				parser->m_nodes[nodeId].boneIndex = parser->m_boneOffsets.size();
				//parser->m_totalBones++;
				parser->m_boneOffsets.push_back(parser->AssimpMat4ToMat4(&mesh->mBones[j]->mOffsetMatrix));
				parser->m_parentBoneId.push_back(-1);
			}
		}
	}
}

void AssimpNodeTraversal(AssimpParser* parser, int parentIndex, aiNode* node, const aiScene* scene)
{
	assert(parser->m_nodeMap.find(node->mName.C_Str()) == parser->m_nodeMap.end());
	parser->m_nodeMap[node->mName.C_Str()] = parser->m_nodes.size();
	parser->m_nodes.push_back({ node,parentIndex });

	auto& back = parser->m_nodes.back();
	back.meshsIndex = node->mMeshes;
	back.numMeshs = node->mNumMeshes;

	back.localTransform = 
			parser->AssimpMat4ToMat4(&back.node->mTransformation);

	int id = parser->m_nodes.size() - 1;
	for (unsigned long long i = 0; i < node->mNumChildren; i++)
	{
		AssimpNodeTraversal(parser, id, node->mChildren[i], scene);
	}
}

void AssimpNodeMapBones(AssimpParser* parser, aiNode* node, const aiScene* scene)
{
	/*MapBones(parser, node, scene);
	for (unsigned long long i = 0; i < node->mNumChildren; i++)
	{
		AssimpNodeMapBones(parser, node->mChildren[i], scene);
	}*/

	for (size_t i = 0; i < parser->m_nodes.size(); i++)
	{
		auto& cnode = parser->m_nodes[i];
		if (cnode.numMeshs == 0) continue;

		for (size_t j = 0; j < cnode.numMeshs; j++)
		{
			auto& mesh = scene->mMeshes[cnode.meshsIndex[j]];
			if (mesh->mNumBones != 0)
			{
				for (size_t k = 0; k < mesh->mNumBones; k++)
				{
					const char* name = mesh->mBones[k]->mName.C_Str();

					auto it = parser->m_nodeMap.find(name);
					assert(it != parser->m_nodeMap.end());

					auto& boneIndex = parser->m_nodes[it->second].boneIndex;

					if (boneIndex == -1)
					{
						boneIndex = parser->m_boneOffsets.size();
						//parser->m_totalBones++;
						parser->m_boneOffsets.push_back(parser->AssimpMat4ToMat4(&mesh->mBones[k]->mOffsetMatrix));
						parser->m_parentBoneId.push_back(-1);
					}
				}
			}
		}
	}
}

void AssimpMapMissingBone(AssimpParser* parser, aiNode* node, const aiScene* scene)
{

}

void MapBoneParent(AssimpParser* parser, aiNode* node, const aiScene* scene)
{
	size_t count = 0;

	bool needFlipYZ = 0;

	for (size_t i = 0; i < node->mNumMeshes; i++)
	{
		auto meshId = node->mMeshes[i];
		auto& mesh = scene->mMeshes[i];

		if (mesh->mNumBones == 0) continue;

		needFlipYZ = true;

		for (size_t j = 0; j < mesh->mNumBones; j++)
		{
			auto& map = parser->m_nodeMap;
			auto it = map.find(mesh->mBones[j]->mName.C_Str());
			assert(it != map.end());
			auto nodeId = it->second;
			if (parser->m_nodes[nodeId].boneIndex != -1)
			{
				if (count == parser->m_parentBoneId.size()) continue;
				parser->m_parentBoneId[count] = parser->m_nodes[parser->m_nodes[nodeId].parentIndex].boneIndex;
				count++;
			}
		}
	}

	/*if (needFlipYZ)
	{
		auto& map = parser->m_nodeMap;
		auto it = map.find(node->mName.C_Str());
		assert(it != map.end());
		auto nodeId = it->second;

		auto& transform = parser->m_nodes[nodeId].localTransform;

		transform *= GetScaleMatrix(2, 2, 2) * GetRotationAxisMatrix({ 1,0,0 }, -PI / 2);
	}*/
}

void AssimpMapBoneParent(AssimpParser* parser, aiNode* node, const aiScene* scene)
{
	MapBoneParent(parser, node, scene);
	for (unsigned long long i = 0; i < node->mNumChildren; i++)
	{
		AssimpMapBoneParent(parser, node->mChildren[i], scene);
	}
}

void ReadAnimations(AssimpParser* parser, const aiScene* scene)
{
	parser->m_animations.resize(scene->mNumAnimations);
	for (size_t i = 0; i < scene->mNumAnimations; i++)
	{
		auto& anim = scene->mAnimations[i];
		auto& parserAnim = parser->m_animations[i];

		parserAnim.aiAnim = anim;
		parserAnim.duration = anim->mDuration;
		parserAnim.ticksPerSecond = anim->mTicksPerSecond;

		for (size_t j = 0; j < anim->mNumChannels; j++)
		{
			parserAnim.channels.push_back({ anim->mChannels[j], 0 });
			parserAnim.channels.back().nodeId = parser->m_nodeMap[anim->mChannels[j]->mNodeName.C_Str()];
		}
	}
}

void DoPreTransform(AssimpParser* parser)
{
	/*for (auto& t : parser->m_nodes)
	{
		t.localTransform *= parser->m_preTransform;
	}*/
	parser->m_nodes[0].localTransform *= parser->m_preTransform;

}

void AssimpParser::Vectorize(const aiScene* scene)
{
	AssimpNodeTraversal(this, -1, scene->mRootNode, scene);
	DoPreTransform(this);
	AssimpNodeMapBones(this, scene->mRootNode, scene);
	//AssimpMapBoneParent(this, scene->mRootNode, scene);
	ReadAnimations(this, scene);
}

Mat4x4 AssimpParser::AssimpMat4ToMat4(aiMatrix4x4* from)
{
	Mat4x4 ret;

	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			ret.m[i][j] = (*from)[j][i];
		}
	}

	return ret;
}

aiMatrix4x4 AssimpParser::Mat4ToAssimpMat4(Mat4x4* from)
{
	aiMatrix4x4 ret;

	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			ret[i][j] = (*from).m[j][i];
		}
	}

	return ret;
}
