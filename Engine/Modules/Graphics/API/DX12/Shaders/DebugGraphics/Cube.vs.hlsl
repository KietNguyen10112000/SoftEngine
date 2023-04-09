#include "../TypeDef.hlsli"

static const float3 CUBE_VERTICES[] =
{
    // front face
    float3(-1.0f,  1.0f, -1.0f),
    float3(1.0f, -1.0f, -1.0f),
    float3(-1.0f, -1.0f, -1.0f),
    float3(1.0f,  1.0f, -1.0f),

    // right side face
    float3(1.0f, -1.0f, -1.0f),
    float3(1.0f,  1.0f,  1.0f),
    float3(1.0f, -1.0f,  1.0f),
    float3(1.0f,  1.0f, -1.0f),

    // left side face
    float3(-1.0f,  1.0f,  1.0f),
    float3(-1.0f, -1.0f, -1.0f),
    float3(-1.0f, -1.0f,  1.0f),
    float3(-1.0f,  1.0f, -1.0f),

    // back face
    float3(1.0f,  1.0f,  1.0f),
    float3(-1.0f, -1.0f,  1.0f),
    float3(1.0f, -1.0f,  1.0f),
    float3(-1.0f,  1.0f,  1.0f),

    // top face
    float3(-1.0f,  1.0f, -1.0f),
    float3(1.0f,  1.0f,  1.0f),
    float3(1.0f,  1.0f, -1.0f),
    float3(-1.0f,  1.0f,  1.0f),

    // bottom face
    float3(1.0f, -1.0f,  1.0f),
    float3(-1.0f, -1.0f, -1.0f),
    float3(1.0f, -1.0f, -1.0f),
    float3(-1.0f, -1.0f,  1.0f)
};

struct VS_INPUT
{
    float4 row1					: INSTANCE_TRANSFORM_ROW1;
    float4 row2					: INSTANCE_TRANSFORM_ROW2;
    float4 row3					: INSTANCE_TRANSFORM_ROW3;
    float4 row4					: INSTANCE_TRANSFORM_ROW4;
    float4 color                : COLOR;
    uint   vertexId             : SV_VertexID;
};

struct VS_OUTPUT
{
    float4 position		: SV_POSITION;
    float4 color        : COLOR;
};

cbuffer CameraCBuffer : register(b0)
{
    CameraData Camera;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

    float4x4 transform = float4x4(input.row1, input.row2, input.row3, input.row4);
    output.position = mul(CUBE_VERTICES[input.vertexId], transform);
	output.position = mul(output.position, Camera.vp);
	output.color = input.color;

	return output;
}