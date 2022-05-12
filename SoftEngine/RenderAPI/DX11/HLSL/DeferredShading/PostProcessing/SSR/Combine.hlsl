struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState defaultSampler			: register(s0);

//always t3
Texture2D depthMap					: register(t3);

Texture2D ssrScene					: register(t4);
Texture2D scene						: register(t5);


float4 main(PS_INPUT input) : SV_TARGET0
{
	float4 color = scene.Sample(defaultSampler, input.textCoord);
	float4 ssr = ssrScene.Sample(defaultSampler, input.textCoord);
	return saturate(color + ssr);
}