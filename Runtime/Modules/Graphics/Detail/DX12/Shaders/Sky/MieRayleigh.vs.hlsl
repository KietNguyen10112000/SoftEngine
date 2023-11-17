#include "../Common/TypeDef.hlsli"

struct VS_OUTPUT
{
	float4 svpos		: SV_POSITION;
	float4 position		: POSITION;
};

cbuffer CameraCBuffer : register(b0, SPACE_VS)
{
	CameraData Camera;
};

const static float2 VERTICES[] = {
	float2(-1,  1),
	float2( 1,  1),
	float2(-1, -1),
	float2( 1,  1),
	float2( 1, -1),
	float2(-1, -1)
};

VS_OUTPUT main(uint vertexId : SV_VertexID)
{
	VS_OUTPUT output;

	float2 vpos = VERTICES[vertexId].xy;
	output.svpos = float4(vpos, 0, 1.0f);

	float4 worldPos = mul(float4(vpos, 1.0f, 1.0f), Camera.inversedVp);
	worldPos /= worldPos.w;
	output.position = worldPos;

	return output;
}