struct PS_INPUT
{
	float4 position		: SV_POSITION;
	float3 color		: COLOR0;
};

SamplerState	defaultSampler			: register(s0);

float4 main(PS_INPUT input) : SV_TARGET
{
	return float4(input.color, 1.0f);
}