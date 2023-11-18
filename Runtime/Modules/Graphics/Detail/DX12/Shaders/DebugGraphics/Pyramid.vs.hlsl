#include "../Common/TypeDef.hlsli"

static const float3 VERTICES[] =
{
float3(0, 0.2, 0),
float3(0.0666667, 0, 0),
float3(0.0539345, 0, 0.0391857),
float3(0, 0.2, 0),
float3(0.0539345, 0, 0.0391857),
float3(0.0206011, 0, 0.0634038),
float3(0, 0.2, 0),
float3(0.0206011, 0, 0.0634038),
float3(-0.0206011, 0, 0.0634038),
float3(0, 0.2, 0),
float3(-0.0206011, 0, 0.0634038),
float3(-0.0539345, 0, 0.0391857),
float3(0, 0.2, 0),
float3(-0.0539345, 0, 0.0391857),
float3(-0.0666667, 0, -5.82819e-09),
float3(0, 0.2, 0),
float3(-0.0666667, 0, -5.82819e-09),
float3(-0.0539345, 0, -0.0391857),
float3(0, 0.2, 0),
float3(-0.0539345, 0, -0.0391857),
float3(-0.0206011, 0, -0.0634038),
float3(0, 0.2, 0),
float3(-0.0206011, 0, -0.0634038),
float3(0.0206011, 0, -0.0634038),
float3(0, 0.2, 0),
float3(0.0206011, 0, -0.0634038),
float3(0.0539345, 0, -0.0391857),
float3(0, 0.2, 0),
float3(0.0539345, 0, -0.0391857),
float3(0.0666667, 0, 4.34455e-08),
float3(0, 0, 0),
float3(0.0666667, 0, 0),
float3(0.0539345, 0, 0.0391857),
float3(0, 0, 0),
float3(0.0539345, 0, 0.0391857),
float3(0.0206011, 0, 0.0634038),
float3(0, 0, 0),
float3(0.0206011, 0, 0.0634038),
float3(-0.0206011, 0, 0.0634038),
float3(0, 0, 0),
float3(-0.0206011, 0, 0.0634038),
float3(-0.0539345, 0, 0.0391857),
float3(0, 0, 0),
float3(-0.0539345, 0, 0.0391857),
float3(-0.0666667, 0, -5.82819e-09),
float3(0, 0, 0),
float3(-0.0666667, 0, -5.82819e-09),
float3(-0.0539345, 0, -0.0391857),
float3(0, 0, 0),
float3(-0.0539345, 0, -0.0391857),
float3(-0.0206011, 0, -0.0634038),
float3(0, 0, 0),
float3(-0.0206011, 0, -0.0634038),
float3(0.0206011, 0, -0.0634038),
float3(0, 0, 0),
float3(0.0206011, 0, -0.0634038),
float3(0.0539345, 0, -0.0391857),
float3(0, 0, 0),
float3(0.0539345, 0, -0.0391857),
float3(0.0666667, 0, 4.34455e-08),
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

    float3 pos = VERTICES[input.vertexId];
    output.position = float4(pos, 1.0f);
    output.position = mul(output.position, transforms[input.instanceId]);
    output.position = mul(output.position, Camera.vp);

    output.color = colors[input.instanceId];

    return output;
}