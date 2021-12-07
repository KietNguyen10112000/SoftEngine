#include "SkyBox.h"

#include <Resource.h>

#include <RenderPipeline.h>

#include <Buffer.h>

SkyCube::SkyCube(const std::wstring& path, uint32_t numArg, void** args)
{
    m_cubeTexture = Resource::Get<TextureCube>(path, numArg, args);

    Vec3 cubeVertices[] =
    {
        Vec3(-1.0f, -1.0f, -1.0f),
        Vec3(-1.0f,  1.0f, -1.0f),
        Vec3(1.0f,  1.0f, -1.0f),
        Vec3(1.0f, -1.0f, -1.0f),

        Vec3(-1.0f, -1.0f, 1.0f),
        Vec3(1.0f, -1.0f, 1.0f),
        Vec3(1.0f,  1.0f, 1.0f),
        Vec3(-1.0f,  1.0f, 1.0f),

        Vec3(-1.0f, 1.0f, -1.0f),
        Vec3(-1.0f, 1.0f,  1.0f),
        Vec3(1.0f, 1.0f,  1.0f),
        Vec3(1.0f, 1.0f, -1.0f),

        Vec3(-1.0f, -1.0f, -1.0f),
        Vec3(1.0f, -1.0f, -1.0f),
        Vec3(1.0f, -1.0f,  1.0f),
        Vec3(-1.0f, -1.0f,  1.0f),

        Vec3(-1.0f, -1.0f,  1.0f),
        Vec3(-1.0f,  1.0f,  1.0f),
        Vec3(-1.0f,  1.0f, -1.0f),
        Vec3(-1.0f, -1.0f, -1.0f),

        Vec3(1.0f, -1.0f, -1.0f),
        Vec3(1.0f,  1.0f, -1.0f),
        Vec3(1.0f,  1.0f,  1.0f),
        Vec3(1.0f, -1.0f,  1.0f)
    };

    unsigned short cubeIndices[] =
    {
        0,  1,  2,
        0,  2,  3,
        4,  5,  6,
        4,  6,  7,
        8,  9, 10,
        8, 10, 11,
        12, 13, 14,
        12, 14, 15,
        16, 17, 18,
        16, 18, 19,
        20, 21, 22,
        20, 22, 23
    };

    m_vertexBuffer = new VertexBuffer(cubeVertices, ARRAYSIZE(cubeVertices), sizeof(Vec3));

    m_indexBuffer = new IndexBuffer(cubeIndices, ARRAYSIZE(cubeIndices), sizeof(unsigned short));

    m_rpl = RenderPipelineManager::Get(
        R"(struct
        {
            Vec3 pos; POSITION, PER_VERTEX #
        };)",
        L"SkyBox/VSSkyCube",
        L"SkyBox/PSSkyCube"
    );
    
}

SkyCube::~SkyCube()
{
    delete m_vertexBuffer;
    delete m_indexBuffer;
    Resource::Release(&m_cubeTexture);
}

void SkyCube::Update(Engine* engine)
{
}

void SkyCube::Render(IRenderer* renderer)
{
    m_transform.SetTranslation(renderer->GetTargetCamera()->GetPosition());
    FlushTransform();
    m_rpl->PSSetResource(m_cubeTexture, 5);
    renderer->Render(m_rpl, m_vertexBuffer, m_indexBuffer);
}
