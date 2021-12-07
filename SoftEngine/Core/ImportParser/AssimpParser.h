#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <map>
#include <Math/Math.h>

class AssimpParser
{
public:
    struct Node
    {
        aiNode* node;
        int parentIndex;
        int boneIndex = -1;

        //indexing mesh, not own
        uint32_t* meshsIndex;
        uint32_t numMeshs;

        Mat4x4 localTransform;
    };

    struct NodeAnim
    {
        aiNodeAnim* nodeAnim;

        //nodeAnim effects to node
        int nodeId;
    };

    struct Animation
    {
        aiAnimation* aiAnim;
        double duration;
        double ticksPerSecond;
        std::vector<NodeAnim> channels;
    };

    //std::vector<int> m_iterOrder;
    std::vector<Node> m_nodes;

    //int m_totalBones = 0;
    //total bone = m_boneOffsets.size()
    std::vector<Mat4x4> m_boneOffsets;
    std::vector<int> m_parentBoneId;
    //map node with index in m_nodes to find node by name
    std::map<std::string, uint32_t> m_nodeMap;

    std::vector<Animation> m_animations;

    Mat4x4 m_preTransform;

public:
    void Vectorize(const aiScene* scene);
    Mat4x4 AssimpMat4ToMat4(aiMatrix4x4* from);
    aiMatrix4x4 Mat4ToAssimpMat4(Mat4x4* from);

};