#include "Model3DBasic.h"

NAMESPACE_BEGIN

Model3DBasic::Model3DBasic(String path) : ResourceBase(path)
{
    auto graphics = Graphics::Get();

    // test a cube first
    Vertex cubeVertices[] =
    {
        { Vec3(-1.0f, -1.0f, -1.0f), Vec2(0, 1) },
        { Vec3(-1.0f,  1.0f, -1.0f), Vec2(0, 0) },
        { Vec3(1.0f,  1.0f, -1.0f), Vec2(1, 0) },
        { Vec3(1.0f, -1.0f, -1.0f), Vec2(1, 1) },

        { Vec3(-1.0f, -1.0f, 1.0f), Vec2(1, 1) },
        { Vec3(1.0f, -1.0f, 1.0f), Vec2(0, 1) },
        { Vec3(1.0f,  1.0f, 1.0f), Vec2(0, 0) },
        { Vec3(-1.0f,  1.0f, 1.0f), Vec2(1, 0) },

        { Vec3(-1.0f, 1.0f, -1.0f), Vec2(0, 1) },
        { Vec3(-1.0f, 1.0f,  1.0f), Vec2(0, 0) },
        { Vec3(1.0f, 1.0f,  1.0f), Vec2(1, 0) },
        { Vec3(1.0f, 1.0f, -1.0f), Vec2(1, 1) },

        { Vec3(-1.0f, -1.0f, -1.0f), Vec2(1, 1) },
        { Vec3(1.0f, -1.0f, -1.0f), Vec2(0, 1) },
        { Vec3(1.0f, -1.0f,  1.0f), Vec2(0, 0) },
        { Vec3(-1.0f, -1.0f,  1.0f), Vec2(1, 0) },

        { Vec3(-1.0f, -1.0f,  1.0f), Vec2(0, 1) },
        { Vec3(-1.0f,  1.0f,  1.0f), Vec2(0, 0) },
        { Vec3(-1.0f,  1.0f, -1.0f), Vec2(1, 0) },
        { Vec3(-1.0f, -1.0f, -1.0f), Vec2(1, 1) },

        { Vec3(1.0f, -1.0f, -1.0f), Vec2(0, 1) },
        { Vec3(1.0f,  1.0f, -1.0f), Vec2(0, 0) },
        { Vec3(1.0f,  1.0f,  1.0f), Vec2(1, 0) },
        { Vec3(1.0f, -1.0f,  1.0f), Vec2(1, 1) },
    };

    // Create index buffer:
    unsigned short cubeIndices[] =
    {
        0,  1,  2,
        0,  2,  3,

        // Back Face
        4,  5,  6,
        4,  6,  7,

        // Top Face
        8,  9, 10,
        8, 10, 11,

        // Bottom Face
        12, 13, 14,
        12, 14, 15,

        // Left Face
        16, 17, 18,
        16, 18, 19,

        // Right Face
        20, 21, 22,
        20, 22, 23
    };

    Vertex vList[36] = {};
    const int vBufferSize = sizeof(vList);

    for (size_t i = 0; i < 36; i++)
    {
        vList[i] = cubeVertices[cubeIndices[i]];
    }

    GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC vbDesc = {};
    vbDesc.count = 36;
    vbDesc.stride = sizeof(Vertex);
    m_vertexBuffer = graphics->CreateVertexBuffer(vbDesc);
    m_vertexBuffer->UpdateBuffer(vList, sizeof(vList));

    m_vertexCount = 36;
}

AABox Model3DBasic::GetLocalAABB() const
{
    return AABox(Vec3::ZERO, Vec3(2, 2, 2));
}

NAMESPACE_END