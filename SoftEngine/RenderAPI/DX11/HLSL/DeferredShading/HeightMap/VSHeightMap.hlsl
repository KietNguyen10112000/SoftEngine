struct VS_INPUT
{
	float3 position			: POSITION;
	float2 textCoord		: TEXTCOORD;
	float3 normal			: NORMAL;
};


struct VS_OUTPUT
{
	float4 pos				: SV_POSITION;
	float4 position			: POSITION;
	float2 textCoord		: TEXTCOORD;
	float3 normal			: NORMAL;
};

cbuffer Transform : register(b0)
{
	row_major float4x4 transform;
};

cbuffer Camera : register(b1)
{
	row_major float4x4 mvp;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	output.pos = mul(float4(input.position, 1.0f), transform);

	output.position = output.pos;

	output.pos = mul(output.pos, mvp);

	output.textCoord = input.textCoord;
	output.normal = input.normal;

	return output;
}