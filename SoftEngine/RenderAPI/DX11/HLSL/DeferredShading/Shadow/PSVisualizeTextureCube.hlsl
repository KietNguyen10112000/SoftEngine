struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float3 textCoord : TEXTCOORD;
};

SamplerState defaultSampler				: register(s0);

//TextureCube cubeMap						: register(t6);
Texture2DArray cubeMap					: register(t6);

/*
 [u v] * [a b] = [u*a + v*c, u*b + v*d] = [u, 1 - v]
         [c d]
*/


float2x2 GetFlipRule(float2 uv, uint faceid)
{
	//flip rules for 6 slices
	const float2x2 flipRules[6] = {
		mul(float2x2(float2(-1,0), float2(1 / uv.y,1)), float2x2(float2(0,1), float2(-1,0))),
		float2x2(float2(0,1), float2(-1,0)),
		float2x2(float2(1,0), float2(0,1)),
		float2x2(float2(-1,0), float2(1 / uv.y,1)),
		float2x2(float2(1,1 / uv.x), float2(0,-1)),
		float2x2(float2(1,0), float2(0,1)),
	};

	return flipRules[faceid];
}

float4 main(PS_INPUT input) : SV_TARGET0
{
	/*float depth = cubeMap.Sample(defaultSampler, input.textCoord);
	depth = pow(depth, 8);
	return float4(depth, depth, depth, 1.0f);*/

	float3 dir = normalize(input.textCoord);
	uint maxIndex = abs(dir.x) > abs(dir.y) ? 0 : 1;
	maxIndex = abs(dir[maxIndex]) > abs(dir.z) ? maxIndex : 2;

	uint faceID = 0;
	if (dir[maxIndex] < 0)
	{
		faceID = 2 * maxIndex + 1;
	}
	else
	{
		faceID = 2 * maxIndex;
	}

	float3 nor = dir / dir[maxIndex];
	float2 coord = float2(0, 0);
	switch (maxIndex)
	{
	case 0:
		coord = nor.yz;
		break;
	case 1:
		coord = nor.xz;
		break;
	case 2:
		coord = nor.xy;
		break;
	}

	float2 uv = coord * 0.5f;

	uv = mul(uv, GetFlipRule(uv, faceID));

	uv += float2(0.5f, 0.5f);

	float depth = cubeMap.Sample(defaultSampler, float3(uv, faceID));

	depth = pow(depth, 8);

	return float4(depth, depth, depth, 1.0f);
}