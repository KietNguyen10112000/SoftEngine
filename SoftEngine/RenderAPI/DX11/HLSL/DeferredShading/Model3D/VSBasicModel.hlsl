struct VS_INPUT
{
	float3 position		: POSITION;
	float2 textCoord	: TEXTCOORD;
	float3 normal		: NORMAL;
};

struct VS_OUTPUT
{
	float4 pos			: SV_POSITION;
	float4 position		: POSITION;
	float2 textCoord	: TEXTCOORD;
	float3 normal		: NORMAL;
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

static float4x4 toWorldSpace = mul(localTransform, transform);

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	output.pos = mul(float4(input.position, 1.0f), toWorldSpace);

	output.position = output.pos;

	output.pos = mul(output.pos, mvp);

	output.textCoord = input.textCoord;

	output.normal = mul(float4(input.normal, 0.0f), toWorldSpace).xyz;

	return output;
}