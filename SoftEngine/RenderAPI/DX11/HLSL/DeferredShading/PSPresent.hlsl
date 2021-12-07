struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState defaultSampler : register(s0);

//Texture2D color							: register(t0);
//Texture2D positionAndSpec				: register(t1);
//Texture2D normAndShininess				: register(t2);


Texture2D depthMap						: register(t3);
Texture2D lastScene						: register(t4);
Texture2D normAndShininess				: register(t6);

float4 main(PS_INPUT input) : SV_TARGET
{
	//float pixelDepth = depthMap.Sample(defaultSampler, input.textCoord);

	//if (pixelDepth > 0.99999f) discard;
	/*float4 pixelColor = color.Sample(defaultSampler, input.textCoord);
	*/

	float4 pixelColor = lastScene.Sample(defaultSampler, input.textCoord);

	//for space coordinate blur effect
	
	//if (pixelDepth > 0.995f)
	//{
		//float4 ns = normAndShininess.Sample(defaultSampler, input.textCoord);
		//if (all(ns.xyz == 0))
		//{
		//	if (pixelColor.x == 1 || pixelColor.y == 1 || pixelColor.z == 1)
		//	{
		//		return pixelColor;
		//	}

		//	float alpha = 0;
		//	//bool flag = false;
		//	if (pixelDepth > 0.999f)
		//	{
		//		alpha = clamp(0.5 - pow(pixelDepth, 512), 0.1f, pixelColor.w);
		//		//flag = true;
		//	}
		//	else
		//	{
		//		alpha = clamp(1 - pow(pixelDepth, 128), 0, pixelColor.w);
		//	}

		//	return float4(pixelColor.xyz, alpha);
		//}
	//}

	return pixelColor;

	//const float gamma = 2.2;
	//const float igamma = 1.0 / gamma;

	//// exposure tone mapping
	//float3 mapped = float3(1.0f, 1.0f, 1.0f) - exp(-pixelColor * 1);
	//// gamma correction 
	//mapped = pow(mapped, float3(igamma, igamma, igamma));

	//return float4(mapped, 0.5f);
}