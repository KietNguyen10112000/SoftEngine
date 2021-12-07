struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float4 position		: POSITION;
	float2 textCoord	: TEXTCOORD;
};

struct PS_OUTPUT
{
	float4 position		: SV_TARGET0;
	float4 color		: SV_TARGET1;
	float4 normAndSpec	: SV_TARGET2;
};

SamplerState defaultSampler : register(s0);

Texture2D diffuse : register(t0);
Texture2D normal : register(t1);

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;

	output.color = diffuse.Sample(defaultSampler, input.textCoord);

	float3 norm = normal.Sample(defaultSampler, input.textCoord).xyz;

	norm = norm * 2 - 1;

	float spec = 512.f;
	output.normAndSpec = float4(norm, spec);

	output.position = input.position;

	return output;
}