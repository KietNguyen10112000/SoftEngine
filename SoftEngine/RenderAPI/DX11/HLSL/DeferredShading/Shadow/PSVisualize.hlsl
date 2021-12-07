struct PS_INPUT
{
	float4 pos			: SV_POSITION; //system value will NOT be interpolated in pixel shader
	//float4 position		: POSITION;
	float2 textCoord	: TEXTCOORD; //this will be be interpolated
};

SamplerState defaultSampler		: register(s0);

Texture2D shadowDepthMap						: register(t2);

Texture2D depthMap								: register(t3);
Texture2D color									: register(t4);
Texture2D positionAndSpec						: register(t5);
Texture2D normAndShininess						: register(t6);

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 ret = float4(0,0,0,0);

	//if (input.textCoord.x < 0.5f && input.textCoord.y < 0.5f)
	//{
	//	ret.x = shadowDepthMap.Sample(defaultSampler, float2(input.textCoord.x * 2.0f, input.textCoord.y * 2.0f));

	//	//if (ret.x != 1) ret.y = 1;
	//	ret.x = pow(ret.x, 8);
	//	ret.y = ret.x;
	//	ret.z = ret.x;

	//	ret.w = 1;
	//}
	/*else if (input.textCoord.x > 0.5f && input.textCoord.y < 0.5f)
	{

	}
	else if (input.textCoord.x < 0.5f && input.textCoord.y > 0.5f)
	{

	}
	else
	{

	}*/

	ret.x = shadowDepthMap.Sample(defaultSampler, input.textCoord);

	ret.x = pow(ret.x, 8);
	ret.y = ret.x;
	ret.z = ret.x;

	ret.w = 1;

	return ret;
}