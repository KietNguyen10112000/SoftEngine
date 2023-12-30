#include "../Common/TypeDef.hlsli"

struct VS_INPUT
{
    float3 position		: POSITION;
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
    row_major float4x4 transform;
    float4 color;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float3 pos = input.position;
    output.position = float4(pos, 1.0f);
    output.position = mul(output.position, transform);
    output.position = mul(output.position, Camera.vp);

    output.color = color;

    return output;
}