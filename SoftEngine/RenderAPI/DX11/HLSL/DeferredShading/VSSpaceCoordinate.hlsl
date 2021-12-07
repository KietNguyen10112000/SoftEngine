struct VS_INPUT
{
	float3 pos			: POSITION;
	float4 color		: COLOR;

};

struct VS_OUTPUT
{
	float4 pos			: SV_POSITION;
	float4 color		: COLOR;
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

	output.pos = mul(float4(input.pos, 1.0f), transform);

	output.pos = mul(output.pos, mvp);

	output.color = input.color;

	return output;
}