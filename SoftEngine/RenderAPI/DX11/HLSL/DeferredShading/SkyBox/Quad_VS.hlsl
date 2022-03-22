struct VS_INPUT
{
	float3 pos : POSITION;
	float2 textCoord: TEXTCOORD;
};

struct VS_OUTPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;

	//frustum corners world position
	float3 position		: POSITION;
};

cbuffer Camera : register(b1)
{
	row_major float4x4 viewProjMat;
	row_major float4x4 invViewProjMat;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	output.pos = float4(input.pos, 1.0f);

	output.textCoord = input.textCoord;

	float4 worldPos = mul(float4(input.pos.xy, 1.0f, 1.0f), invViewProjMat);
	worldPos /= worldPos.w;
	output.position = worldPos.xyz;

	return output;
}