struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float4 color		: COLOR;
};

struct PS_OUTPUT
{
	float4 color		: SV_TARGET0;
	float4 position		: SV_TARGET1;
	float4 normAndSpec	: SV_TARGET2;
};

SamplerState defaultSampler : register(s0);

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;

	output.color = input.color;

	float3 norm = float3(0, 0, 0);
	float spec = 0;
	output.normAndSpec = float4(norm, spec);

	output.position = float4(0, 0, 0, 0);

	return output;
}