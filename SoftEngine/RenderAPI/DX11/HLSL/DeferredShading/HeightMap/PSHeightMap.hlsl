struct PS_INPUT
{
	float4 pos				: SV_POSITION;
	float4 position			: POSITION;
	float2 textCoord		: TEXTCOORD;
	float3 normal			: NORMAL;
};

struct PS_OUTPUT
{
	float4 color				: SV_TARGET0;
	float4 positionAndSpec		: SV_TARGET1;
	float4 normAndShininess		: SV_TARGET2;
};

SamplerState defaultSampler : register(s0);

Texture2D diffuse : register(t0);


PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;

	output.color = diffuse.Sample(defaultSampler, input.textCoord);

	output.normAndShininess = float4(input.normal, 512.0f);

	output.positionAndSpec = float4(input.position.xyz, 0.8f);

	return output;
}