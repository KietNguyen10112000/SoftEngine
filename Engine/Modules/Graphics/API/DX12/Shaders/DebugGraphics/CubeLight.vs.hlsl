#include "../TypeDef.hlsli"

#include "../Primitives/CubeVertices.hlsli"

struct VS_INPUT
{
    uint   vertexId             : SV_VertexID;
};

struct VS_OUTPUT
{
    float4 position		: SV_POSITION;
    float4 color        : COLOR;

    float4 pos          : WORLD_POSITION;
    float4 normal       : WORLD_NORMAL;
};

cbuffer SceneCBuffer : register(b0)
{
    SceneData Scene;
};

cbuffer CameraCBuffer : register(b1)
{
    CameraData Camera;
};

cbuffer ObjectCBuffer : register(b2)
{
    ObjectData Object;
    float4     Color;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    uint triangleId = input.vertexId / 3;
    uint vId = triangleId * 3;
    float3 v1 = CUBE_VERTICES[CUBE_INDICES[vId]];
    float3 v2 = CUBE_VERTICES[CUBE_INDICES[vId + 1]];
    float3 v3 = CUBE_VERTICES[CUBE_INDICES[vId + 2]];

    float3 n = cross(v2 - v1, v3 - v1);
    output.normal = mul(float4(n, 0.0f), Object.transform);

    float3 pos = CUBE_VERTICES[CUBE_INDICES[input.vertexId]];
    output.position = float4(pos, 1.0f);
    output.position = mul(output.position, Object.transform);

    output.position /= output.position.w;
    output.pos = output.position;

    output.position = mul(output.position, Camera.vp);

    output.color = Color;

    return output;
}