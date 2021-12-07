#pragma once

//cal TBN for v1
void CalTangentBitangent(
    const Vec3& v1, const Vec2& uv1,
    const Vec3& v2, const Vec2& uv2,
    const Vec3& v3, const Vec2& uv3,
    Vec3* outTangent, Vec3* outBitangent)
{
    auto& tangent = *outTangent;
    auto& bitangent = *outBitangent;

    Vec3 edge1 = v2 - v1;
    Vec3 edge2 = v3 - v1;

    Vec2 duv1 = uv2 - uv1;
    //duv1.y = -duv1.y;
    Vec2 duv2 = uv3 - uv1;
    //duv2.y = -duv2.y;

    float f = 1.0f / (duv1.x * duv2.y - duv2.x * duv1.y);

    tangent.x = f * (duv2.y * edge1.x - duv1.y * edge2.x);
    tangent.y = f * (duv2.y * edge1.y - duv1.y * edge2.y);
    tangent.z = f * (duv2.y * edge1.z - duv1.y * edge2.z);

    bitangent.x = f * (-duv2.x * edge1.x + duv1.x * edge2.x);
    bitangent.y = f * (-duv2.x * edge1.y + duv1.x * edge2.y);
    bitangent.z = f * (-duv2.x * edge1.z + duv1.x * edge2.z);

    tangent.Normalize();
    bitangent.Normalize();
}

void RandomTangentBitangent(const Vec3& normal, Vec3* outTangent, Vec3* outBitangent)
{
    auto& tangent = *outTangent;
    auto& bitangent = *outBitangent;

    Vec3 c1 = CrossProduct(normal, Vec3(0.0, 0.0, 1.0));
    Vec3 c2 = CrossProduct(normal, Vec3(0.0, 1.0, 0.0));

    if (c1.Length() > c2.Length())
    {
        tangent = c1;
    }
    else
    {
        tangent = c2;
    }

    tangent.Normalize();

    bitangent = CrossProduct(tangent, normal);
    bitangent.Normalize();
}

//fbx_sdk port
class FBXImporter
{
public:
    static void ConvertMat4x4(FbxAMatrix* from, Mat4x4* to);

    template <typename T, typename _Fnc, typename ... _Args>
    static void Traversal(FbxManager* lSdkManager, FbxScene* scene,
        T* init, Mat4x4& preTransform, _Fnc processMeshFnc, _Args&& ...args);

    template <typename T, typename _Fnc, typename ... _Args>
    static bool ProcessNode(FbxNode* node, Mat4x4 localTransform,
        T* init, Mat4x4& preTransform, _Fnc processMeshFnc, _Args&& ...args);


    static void TBNAnimModel_ProcessMesh(Mat4x4& localTransform, AnimModel* init, FbxMesh* mesh);

};

void FBXImporter::ConvertMat4x4(FbxAMatrix* from, Mat4x4* to)
{

}

template<typename T, typename _Fnc, typename ..._Args>
void FBXImporter::Traversal(FbxManager* lSdkManager, FbxScene* scene,
    T* init, Mat4x4& preTransform, _Fnc processMeshFnc, _Args&& ...args)
{
    FbxGeometryConverter converter = FbxGeometryConverter(lSdkManager);

    assert(converter.Triangulate(scene, true));

    FbxNode* rootNode = scene->GetRootNode();

    auto count = rootNode->GetChildCount();
    assert(count >= 0);

    Mat4x4 local;
    for (size_t i = 0; i < count; i++)
    {
        FbxAMatrix localTransfbx = rootNode->EvaluateLocalTransform();

        ConvertMat4x4(&localTransfbx, &local);

        ProcessNode(rootNode->GetChild(i), local, init, preTransform, processMeshFnc, std::forward<_Args>(args) ...);
    }
}

template<typename T, typename _Fnc, typename ..._Args>
bool FBXImporter::ProcessNode(FbxNode* node, Mat4x4 transform,
    T* init, Mat4x4& preTransform, _Fnc processMeshFnc, _Args&& ...args)
{

    if (node->GetNodeAttribute() == NULL)
    {
        FBXSDK_printf("NULL Node Attribute\n\n");
    }
    else
    {
        auto attr = node->GetNodeAttribute();
        FbxNodeAttribute::EType aType = attr->GetAttributeType();

        switch (aType)
        {
        case fbxsdk::FbxNodeAttribute::eMarker:
            break;
        case fbxsdk::FbxNodeAttribute::eSkeleton:
            break;
        case fbxsdk::FbxNodeAttribute::eMesh:
            processMeshFnc(transform, init, (FbxMesh*)attr);
            break;
        case fbxsdk::FbxNodeAttribute::eNurbs:
            break;
        case fbxsdk::FbxNodeAttribute::ePatch:
            break;
        case fbxsdk::FbxNodeAttribute::eNurbsCurve:
            break;
        case fbxsdk::FbxNodeAttribute::eTrimNurbsSurface:
            break;
        case fbxsdk::FbxNodeAttribute::eBoundary:
            break;
        case fbxsdk::FbxNodeAttribute::eNurbsSurface:
            break;
        case fbxsdk::FbxNodeAttribute::eShape:
            break;
        case fbxsdk::FbxNodeAttribute::eLine:
            break;
        default:
            break;
        }
    }

    auto count = node->GetChildCount();
    assert(count >= 0);

    Mat4x4 local;
    FbxAMatrix localTransfbx = node->EvaluateLocalTransform();
    ConvertMat4x4(&localTransfbx, &local);

    for (size_t i = 0; i < count; i++)
    {
        ProcessNode(node->GetChild(i), transform * local, init, preTransform, processMeshFnc, std::forward<_Args>(args) ...);
    }

    return true;
}

void TBNAnimModel_GetData_GenerateTBN(
    std::vector<TBNAnimModel::NonAnimMesh::Vertex>& verticesOut, std::vector<uint32_t>& indicesOut,
    FbxMesh* mesh)
{
    FbxVector4* vertices = mesh->GetControlPoints();
    FbxLayerElementArrayTemplate<FbxVector2>* uvVertices = 0;
    mesh->GetTextureUV(&uvVertices);

    FbxLayerElementArrayTemplate<FbxVector4>* normals = 0;
    mesh->GetNormals(&normals);

    auto vcount = mesh->GetControlPointsCount();

    using Vertex = TBNAnimModel::NonAnimMesh::Vertex;

    Vec3 tangent;
    Vec3 bitangent;

    Vertex triangle[3] = {};

    auto cmp = [](const Vertex& v1, const Vertex& v2)->bool { return ::memcmp(&v1, &v2, sizeof(Vertex)) > 0; };

    std::map<Vertex, size_t, decltype(cmp)> vmap(cmp);

    auto icount = mesh->GetPolygonCount();

    for (size_t i = 0; i < icount; i++)
    {
        for (size_t j = 0; j < 3; j++)
        {
            auto id = mesh->GetPolygonVertex(i, j);
            auto uvID = mesh->GetTextureUVIndex(i, j);

            Vertex& vertex = triangle[j];
            vertex.pos.x = (float)vertices[id].mData[0];
            vertex.pos.y = (float)vertices[id].mData[1];
            vertex.pos.z = (float)vertices[id].mData[2];

            vertex.texcoord.x = (float)uvVertices->GetAt(uvID).mData[0];
            vertex.texcoord.y = 1.0f - (float)uvVertices->GetAt(uvID).mData[1];
        }

        CalTangentBitangent(
            triangle[0].pos, triangle[0].texcoord,
            triangle[1].pos, triangle[1].texcoord,
            triangle[2].pos, triangle[2].texcoord,
            &tangent, &bitangent
        );

        for (size_t j = 0; j < 3; j++)
        {
            Vertex& vertex = triangle[j];

            vertex.tangent = tangent;
            vertex.bitangent = bitangent;

            auto it = vmap.find(vertex);

            if (it == vmap.end())
            {
                vmap.insert({ vertex, verticesOut.size() });
                indicesOut.push_back(verticesOut.size());
                verticesOut.push_back(vertex);
            }
            else
            {
                indicesOut.push_back(it->second);
            }
        }
    }

}

void TBNAnimModel_GetData(
    std::vector<TBNAnimModel::NonAnimMesh::Vertex>& verticesOut, std::vector<uint32_t>& indicesOut,
    FbxMesh* mesh)
{
    FbxVector4* vertices = mesh->GetControlPoints();
    FbxLayerElementArrayTemplate<FbxVector2>* uvVertices = 0;
    mesh->GetTextureUV(&uvVertices);

    FbxLayerElementArrayTemplate<FbxVector4>* binormals = 0;
    FbxLayerElementArrayTemplate<FbxVector4>* tangents = 0;

    mesh->GetBinormals(&binormals);
    mesh->GetTangents(&tangents);

    auto vcount = mesh->GetControlPointsCount();

    using Vertex = TBNAnimModel::NonAnimMesh::Vertex;

    auto cmp = [](const Vertex& v1, const Vertex& v2)->bool { return ::memcmp(&v1, &v2, sizeof(Vertex)) > 0; };

    std::map<Vertex, size_t, decltype(cmp)> vmap(cmp);

    auto icount = mesh->GetPolygonCount();

    for (size_t i = 0; i < icount; i++)
    {
        for (size_t j = 0; j < 3; j++)
        {
            auto id = mesh->GetPolygonVertex(i, j);
            auto uvID = mesh->GetTextureUVIndex(i, j);

            Vertex vertex = {};
            vertex.pos.x = (float)vertices[id].mData[0];
            vertex.pos.y = (float)vertices[id].mData[1];
            vertex.pos.z = (float)vertices[id].mData[2];

            vertex.texcoord.x = (float)uvVertices->GetAt(uvID).mData[0];
            vertex.texcoord.y = 1.0f - (float)uvVertices->GetAt(uvID).mData[1];

            vertex.tangent.x = (float)tangents->GetAt(uvID).mData[0];
            vertex.tangent.y = (float)tangents->GetAt(uvID).mData[1];
            vertex.tangent.z = (float)tangents->GetAt(uvID).mData[2];
            vertex.tangent.Normalize();

            vertex.bitangent.x = (float)binormals->GetAt(uvID).mData[0];
            vertex.bitangent.y = (float)binormals->GetAt(uvID).mData[1];
            vertex.bitangent.z = (float)binormals->GetAt(uvID).mData[2];
            vertex.bitangent.Normalize();

            auto it = vmap.find(vertex);

            if (it == vmap.end())
            {
                vmap.insert({ vertex, verticesOut.size() });
                indicesOut.push_back(verticesOut.size());
                verticesOut.push_back(vertex);
            }
            else
            {
                indicesOut.push_back(it->second);
            }
        }
    }
}

void FBXImporter::TBNAnimModel_ProcessMesh(Mat4x4& localTransform, AnimModel* init, FbxMesh* mesh)
{
    using Vertex = TBNAnimModel::NonAnimMesh::Vertex;
    std::vector<Vertex> verticesOut;
    std::vector<uint32_t> indicesOut;

    if (mesh->GetElementBinormalCount() < 1)
    {
        TBNAnimModel_GetData_GenerateTBN(verticesOut, indicesOut, mesh);
        goto endLabel;
    }

    TBNAnimModel_GetData(verticesOut, indicesOut, mesh);

endLabel:
    auto vb = new VertexBuffer(&verticesOut[0], verticesOut.size(), sizeof(Vertex));
    auto ib = new IndexBuffer(&indicesOut[0], indicesOut.size(), sizeof(uint32_t));

    init->m_renderBuf.push_back({ vb,ib,0 });
}
