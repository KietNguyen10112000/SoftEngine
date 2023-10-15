#include "../Common/Common.hlsli"

struct PS_INPUT
{
	float4 position		: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState	defaultSampler			: register(s0);

Texture2D		color					: register(t0, SPACE_PS);

float4 main(PS_INPUT input) : SV_TARGET
{
	return color.Sample(defaultSampler, input.textCoord).rgba;
	//return float4(input.textCoord, 1.0f, 1.0f);
	//return float4(color.Sample(defaultSampler, input.textCoord).rgb, 1.0f);
}