#pragma once

#include <Buffer.h>

#include <Shader.h>

#include <Math/Math.h>
#include <Resource.h>

#include <RenderPipeline.h>

#include <IObject.h>

struct Vertex
{
    Vec3 pos;
    Vec2 textCoord;
};


class Cube : public IRenderableObject
{
public:
    inline static VertexBuffer* cubeVertexBuffer = nullptr;
    inline static IndexBuffer* cubeIndices = nullptr;
    inline static unsigned int count = 0;

public:
    inline Cube(float x, float y, float z, float sX, float sY, float sZ,
        float rX, float rY, float rZ, const wchar_t* texturePath);
    inline ~Cube();

public:
    inline virtual void Update(Engine* engine) override {};
    inline virtual void Render(IRenderer* renderer) override;

};

inline Cube::Cube(float x, float y, float z, float sX, float sY, float sZ,
    float rX, float rY, float rZ, const wchar_t* texturePath)
{
    Mat4x4 temp;
    m_transform.SetIdentity();
    m_transform.SetScale(sX, sY, sZ);

    m_transform *= temp.SetRotationX(rX);
    m_transform *= temp.SetRotationY(rY);
    m_transform *= temp.SetRotationZ(rZ);

    m_transform *= temp.SetTranslation(x, y, z);

    if (!cubeVertexBuffer)
    {
        Vertex CubeVertices[] =
        {
            {Vec3(-1.0f, -1.0f, -1.0f), Vec2(0,   1)},
            {Vec3(-1.0f,  1.0f, -1.0f), Vec2(0,   0)},
            {Vec3(1.0f,  1.0f, -1.0f), Vec2(1,   0)},
            {Vec3(1.0f, -1.0f, -1.0f), Vec2(1,   1)},

            {Vec3(-1.0f, -1.0f, 1.0f), Vec2(1,   1)},
            {Vec3(1.0f, -1.0f, 1.0f), Vec2(0,   1)},
            {Vec3(1.0f,  1.0f, 1.0f), Vec2(0,   0)},
            {Vec3(-1.0f,  1.0f, 1.0f), Vec2(1,   0)},

            {Vec3(-1.0f, 1.0f, -1.0f), Vec2(0,   1)},
            {Vec3(-1.0f, 1.0f,  1.0f), Vec2(0,   0)},
            {Vec3(1.0f, 1.0f,  1.0f), Vec2(1,   0)},
            {Vec3(1.0f, 1.0f, -1.0f), Vec2(1,   1)},

            {Vec3(-1.0f, -1.0f, -1.0f), Vec2(1,   1)},
            {Vec3(1.0f, -1.0f, -1.0f), Vec2(0,   1)},
            {Vec3(1.0f, -1.0f,  1.0f), Vec2(0,   0)},
            {Vec3(-1.0f, -1.0f,  1.0f), Vec2(1,   0)},

            {Vec3(-1.0f, -1.0f,  1.0f), Vec2(0,   1)},
            {Vec3(-1.0f,  1.0f,  1.0f), Vec2(0,   0)},
            {Vec3(-1.0f,  1.0f, -1.0f), Vec2(1,   0)},
            {Vec3(-1.0f, -1.0f, -1.0f), Vec2(1,   1)},

            {Vec3(1.0f, -1.0f, -1.0f), Vec2(0,   1)},
            {Vec3(1.0f,  1.0f, -1.0f), Vec2(0,   0)},
            {Vec3(1.0f,  1.0f,  1.0f), Vec2(1,   0)},
            {Vec3(1.0f, -1.0f,  1.0f), Vec2(1,   1)},
        };

        // Create index buffer:
        unsigned short CubeIndices[] =
        {

            0,  1,  2,
            0,  2,  3,

            // Back Face
            4,  5,  6,
            4,  6,  7,

            // Top Face
            8,  9, 10,
            8, 10, 11,

            //// Bottom Face
            12, 13, 14,
            12, 14, 15,

            // Left Face
            16, 17, 18,
            16, 18, 19,

            // Right Face
            20, 21, 22,
            20, 22, 23

        };

        int m_indexCount = ARRAYSIZE(CubeIndices);

        cubeVertexBuffer = new VertexBuffer(CubeVertices, ARRAYSIZE(CubeVertices), sizeof(Vertex));

        cubeIndices = new IndexBuffer(CubeIndices, ARRAYSIZE(CubeIndices), sizeof(unsigned short));
    }

    count++;

    m_rpl = RenderPipelineManager::Get(
        R"(struct Vertex
		{
			Vec3 pos; POSITION, PER_VERTEX #
			Vec2 textCoord; TEXTCOORD, PER_VERTEX #
		};)", 
        L"VSModel", 
        L"PSModel"
    );

    m_rpl->VSSetVar(shaderTransform, OBJECT_TRANSFROM_SHADER_LOCATION);
}

inline Cube::~Cube()
{
    count--;
    if (count == 0)
    {
        delete cubeVertexBuffer;
        delete cubeIndices;
        cubeVertexBuffer = nullptr;
        cubeIndices = nullptr;
    }
}

inline void Cube::Render(IRenderer* renderer)
{
    shaderTransform->Update(&m_transform, sizeof(Mat4x4));
    renderer->Render(m_rpl, cubeVertexBuffer, cubeIndices);
}
