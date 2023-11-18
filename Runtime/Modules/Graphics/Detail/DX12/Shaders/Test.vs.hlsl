//#define D3DCOMPILE_DEBUG 1

#include "Common/TypeDef.hlsli"

struct VS_INPUT
{
	float3 position		: POSITION;
	float3 tangent		: TANGENT;
	float3 bitangent	: BITANGENT;
	float3 normal		: NORMAL;
	float2 textCoord	: TEXTCOORD;
};

struct VS_OUTPUT
{
	float4 svposition	: SV_POSITION;
	float3 position		: POSITION;
	float3x3 TBN		: TBN_MATRIX;
	float2 textCoord	: TEXTCOORD;
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

	output.svposition = mul(float4(input.position, 1.0f), Object.transform);
	output.position = output.svposition / output.svposition.w;

	output.svposition = mul(output.svposition, Camera.vp);

	float3 t = normalize(mul(float4(input.tangent, 0.0), Object.transform).xyz);
	float3 b = normalize(mul(float4(input.bitangent, 0.0), Object.transform).xyz);
	float3 n = normalize(mul(float4(input.normal, 0.0), Object.transform).xyz);

	output.TBN = float3x3(t, b, n);

	output.textCoord = input.textCoord;

	return output;
}