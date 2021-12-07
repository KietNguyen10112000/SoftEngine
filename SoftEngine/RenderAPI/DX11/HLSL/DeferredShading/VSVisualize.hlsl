struct VS_INPUT
{
	float3 pos : POSITION;
	float2 textCoord: TEXTCOORD;
};

struct VS_OUTPUT
{
	float4 pos			: SV_POSITION;
	//float4 position		: POSITION;
	float2 textCoord	: TEXTCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	output.pos = float4(input.pos, 1.0f);

	output.textCoord = input.textCoord;

	return output;
}