#include "../Common/TypeDef.hlsli"

struct VS_INPUT
{
    float3 position		: POSITION;
	float3 tangent		: TANGENT;
	float3 bitangent	: BITANGENT;
	float3 normal		: NORMAL;
	float2 textCoord	: TEXTCOORD;
    
    uint vertexId       : SV_VertexID;
    uint instanceId     : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 position     : SV_POSITION;
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

    float3 pos = input.position;
    output.position = float4(pos, 1.0f);
    output.position = mul(output.position, transforms[input.instanceId]);
    output.position = mul(output.position, Camera.vp);

    output.color = colors[input.instanceId];

    return output;
}