#define MAX_LIGHT_SHADOW 10

struct PS_INPUT
{
	float4 pos							: SV_POSITION;

	//position from diffrent view
	//float depth							: POSITIONS;
};


//cbuffer LightSystemInfo : register(b3)
//{
//	//num light make shadow
//	uint numberLight;
//	float3 padding;
//};

//SV_DepthLessEqual
float main(PS_INPUT input) : SV_DepthLessEqual
{
	//float minValue = 2.0f;

	//[loop] for (uint i = 0; i < numberLight; i++)
	//{
	//	minValue = min(input.depth[i], minValue);
	//}

	return 0;
}