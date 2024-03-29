#include "../Common/TypeDef.hlsli"

static const float3 CUBE_VERTICES[] =
{
    float3(-1.0f, -1.0f, -1.0f),
    float3(-1.0f,  1.0f, -1.0f),
    float3(1.0f,  1.0f, -1.0f),
    float3(1.0f, -1.0f, -1.0f),

    float3(-1.0f, -1.0f, 1.0f),
    float3(1.0f, -1.0f, 1.0f),
    float3(1.0f,  1.0f, 1.0f),
    float3(-1.0f,  1.0f, 1.0f),

    float3(-1.0f, 1.0f, -1.0f),
    float3(-1.0f, 1.0f,  1.0f),
    float3(1.0f, 1.0f,  1.0f),
    float3(1.0f, 1.0f, -1.0f),

    float3(-1.0f, -1.0f, -1.0f),
    float3(1.0f, -1.0f, -1.0f),
    float3(1.0f, -1.0f,  1.0f),
    float3(-1.0f, -1.0f,  1.0f),

    float3(-1.0f, -1.0f,  1.0f),
    float3(-1.0f,  1.0f,  1.0f),
    float3(-1.0f,  1.0f, -1.0f),
    float3(-1.0f, -1.0f, -1.0f),

    float3(1.0f, -1.0f, -1.0f),
    float3(1.0f,  1.0f, -1.0f),
    float3(1.0f,  1.0f,  1.0f),
    float3(1.0f, -1.0f,  1.0f)
};

static const uint CUBE_INDICES[] =
{
    // Front face
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

struct VS_INPUT
{
    uint   vertexId             : SV_VertexID;
    uint   instanceId           : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 position		: SV_POSITION;
    float4 color        : COLOR;
};

cbuffer CameraCBuffer : register(b0, SPACE_VS)
{
    CameraData Camera;
};

cbuffer DataCBuffer : register(b1, SPACE_VS)
{
    row_major float4x4 transforms[512];
    float4 colors[512];
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float3 pos = CUBE_VERTICES[CUBE_INDICES[input.vertexId]];
    output.position = float4(pos, 1.0f);
    output.position = mul(output.position, transforms[input.instanceId]);
    output.position = mul(output.position, Camera.vp);

    output.color = colors[input.instanceId];

    return output;
}