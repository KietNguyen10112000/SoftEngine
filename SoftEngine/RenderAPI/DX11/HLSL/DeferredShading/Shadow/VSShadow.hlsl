//for directional and spot light only

struct VS_INPUT
{
	float3 position		: POSITION;
	float2 textCoord	: TEXTCOORD;
};

struct VS_OUTPUT
{
	float4 pos						: SV_POSITION;
};

cbuffer Transform : register(b0)
{
	row_major float4x4 transform;
};

cbuffer Camera : register(b1)
{
	row_major float4x4 mvp;
};

cbuffer LocalTransform : register(b2)
{
	row_major float4x4 localTransform;
};

cbuffer LightShadow : register(b3)
{
	row_major float4x4 lightVP;
};

static float4x4 toWorldSpace = mul(localTransform, transform);

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	output.pos = mul(float4(input.position, 1.0f), toWorldSpace);

	output.pos = mul(output.pos, lightVP);

	return output;
}