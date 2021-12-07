struct PS_INPUT
{
	float4 pos			: SV_POSITION; //system value will NOT be interpolated in pixel shader
	//float4 position		: POSITION;
	float2 textCoord	: TEXTCOORD; //this will be be interpolated
};

SamplerState defaultSampler		: register(s0);

Texture2D depthMap								: register(t3);
Texture2D color									: register(t4);
Texture2D positionAndSpec						: register(t5);
Texture2D normAndShininess						: register(t6);

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 ret = float4(0,0,0,0);

	if (input.textCoord.x < 0.5f && input.textCoord.y < 0.5f)
	{
		float2 tc = float2(input.textCoord.x * 2.0f, input.textCoord.y * 2.0f);

		//visualize position
		ret = positionAndSpec.Sample(defaultSampler, tc);

		float depth = depthMap.Sample(defaultSampler, tc);

		if(depth < 0.9999f)
			ret = float4(normalize(ret.xyz), 1.0f);

		///ret = float4(pow(depth, 16), 0, 0, 1);
	}
	else if (input.textCoord.x > 0.5f && input.textCoord.y < 0.5f)
	{
		//visualize color
		ret = color.Sample(defaultSampler,
			float2((input.textCoord.x - 0.5f) * 2.0f, input.textCoord.y * 2.0f));
		//ret = float4(0, 1, 0, 1);
	}
	else if (input.textCoord.x < 0.5f && input.textCoord.y > 0.5f)
	{
		//visualize normAndSpec
		float3 norm = normAndShininess.Sample(defaultSampler,
			float2(input.textCoord.x * 2.0f, (input.textCoord.y - 0.5f) * 2.0f)).xyz;

		norm = (normalize(norm) + 1) / 2.0f;

		ret = float4(norm, 1);
	}
	else
	{
		//ret = lastScene.Sample(defaultSampler,
			//float2((input.textCoord.x - 0.5f) * 2.0f, (input.textCoord.y - 0.5f) * 2.0f));
		//ret = float4(0, 1, 1, 1);
		//visualize normAndSpec
		float spec = positionAndSpec.Sample(defaultSampler,
			float2((input.textCoord.x - 0.5f) * 2.0f, (input.textCoord.y - 0.5f) * 2.0f)).w;

		ret = float4(spec, spec, spec, 1);

	}

	return ret;
}