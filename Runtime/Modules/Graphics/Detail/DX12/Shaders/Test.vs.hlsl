#include "Common/TypeDef.hlsli"

struct VS_INPUT
{
	float3 position		: POSITION;
	float3 color		: COLOR0;
};

struct VS_OUTPUT
{
	float4 position		: SV_POSITION;
	float3 color		: COLOR0;
};

cbuffer CameraCBuffer : register(b0, space0)
{
	CameraData Camera;
};

cbuffer ObjectCBuffer : register(b1, space0)
{
	ObjectData Object;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.position = mul(float4(input.position, 1.0f), Object.transform);
	output.position = mul(output.position, Camera.vp);
	output.color = input.color;

	return output;
}