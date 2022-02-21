struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState defaultSampler			: register(s0);


Texture2D scene					: register(t4);

float4 main(PS_INPUT input) : SV_TARGET0
{
	float3 pixelColor = scene.Sample(defaultSampler, input.textCoord).rgb;

	return float4(pixelColor, 1.0f);
}