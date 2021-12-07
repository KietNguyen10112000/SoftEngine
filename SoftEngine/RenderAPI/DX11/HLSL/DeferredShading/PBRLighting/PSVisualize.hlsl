struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD; 
};

SamplerState defaultSampler						: register(s0);

Texture2D depthMap								: register(t3);
Texture2D color									: register(t4);
Texture2D positionAndSpec						: register(t5);
Texture2D normAndShininess						: register(t6);
Texture2D metallicRoughnessAO					: register(t7);


float4 main(PS_INPUT input) : SV_TARGET
{
	float4 ret = float4(0,0,0,0);

	float4 mra = metallicRoughnessAO.Sample(defaultSampler, input.textCoord);

	if (input.textCoord.x < 0.5f && input.textCoord.y < 0.5f)
	{
		ret = float4(mra.x, mra.x, mra.x, 1);
	}
	else if (input.textCoord.x > 0.5f && input.textCoord.y < 0.5f)
	{
		ret = float4(mra.y, mra.y, mra.y, 1);
	}
	else if (input.textCoord.x < 0.5f && input.textCoord.y > 0.5f)
	{
		ret = float4(mra.z, mra.z, mra.z, 1);
	}
	/*else
	{

	}*/

	return ret;
}