#define D3DCOMPILE_DEBUG 1

struct PS_INPUT
{
	float4 position		: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState	defaultSampler			: register(s0);

Texture2D		color					: register(t0, space1);

float4 main(PS_INPUT input) : SV_TARGET
{
	return float4(color.Sample(defaultSampler, input.textCoord).rgb, 1.0f);
}