#include "../Common/Common.hlsli"

struct VS_OUTPUT
{
	float4 position		: SV_POSITION;
	float2 uv			: TEXTCOORD;
};

cbuffer CameraCBuffer : register(b0, SPACE_VS)
{
	float4 vertices[36];
};

const static float2 UV[] = {
	float2(0, 0),
	float2(1, 0),
	float2(0, 1),
	float2(1, 0),
	float2(1, 1),
	float2(0, 1)
};

VS_OUTPUT main(uint vertexId : SV_VertexID)
{
	VS_OUTPUT output;

	output.position = float4(vertices[vertexId].xy, 0, 1.0f);
	output.uv = UV[vertexId];

	return output;
}