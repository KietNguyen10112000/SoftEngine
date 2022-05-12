struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

SamplerState defaultSampler				: register(s0);

Texture2D depthMap						: register(t3);
Texture2D positionMap					: register(t4);
Texture2D normalMap						: register(t5);
Texture2D scene							: register(t6);


cbuffer Camera : register(b0)
{
	float3 viewPoint;
	float _CameraPadding1;

	row_major float4x4 viewProjMat;
	row_major float4x4 invViewProjMat;
};


#define NUM_SAMPLES 32
#define MAX_RAY_LENGTH 64.0f

#define BINARY_SEARCH_MAX_STEPS 128

#define LOOP [loop]

// isIntersect == true if BinarySearch() find the intersection point successfully
float3 BinarySearch(float3 start, float3 end, float4x4 viewProj, out bool isIntersect)
{
	isIntersect = false;

	LOOP for (int i = 0; i < BINARY_SEARCH_MAX_STEPS; i++)
	{
		float3 center = (start + end) / 2.0f;

		float4 ret = mul(float4(center, 1.0f), viewProj);
		float3 screenPos = ret.xyz / ret.w;
		screenPos.x = screenPos.x * 0.5f + 0.5f;
		screenPos.y = -screenPos.y * 0.5f + 0.5f;

		float screenDepth = depthMap.Sample(defaultSampler, screenPos.xy).x;

		if (screenDepth == screenPos.z)
		{
			isIntersect = true;
			return end;
		}

		// ray off screen => follow start
		if (screenDepth < screenPos.z)
		{
			end = center;
		}
		// ray on screen => follow end
		else
		{
			start = center;
		}

		//if (distance(start, end) < 0.01f /*all(abs(start - end)) < 0.01f*/)
		//{
			/*if (abs(screenDepth - screenPos.z) < 0.000001f)
			{
				isIntersect = true;
				return end;
			}*/
			//return 0.0f.xxx;
		//}
	}

	return 0.5f.xxx;
}

//const static float ditherPattern[4][4] = {
//	{ 0.0f, 0.5f, 0.125f, 0.625f},
//	{ 0.75f, 0.22f, 0.875f, 0.375f},
//	{ 0.1875f, 0.6875f, 0.0625f, 0.5625},
//	{ 0.9375f, 0.4375f, 0.8125f, 0.3125}
//};

// return color
float3 SSR(float3 position, float3 normal, float3 eye, float4x4 viewProj, float2 uv)
{
	/*int x = floor(uv.x * 768.0f);
	int y = floor(uv.y * 432.0f);
	float ditherValue = ditherPattern[x % 4][y % 4];*/

	float3 V = normalize(position - eye);
	float3 R = normalize(reflect(V, normal));

	float3 dR = (MAX_RAY_LENGTH / NUM_SAMPLES) * R;
	float3 currentPos = position /*+ ditherValue * dR*/;

	LOOP for (int i = 0; i < NUM_SAMPLES; i++)
	{
		currentPos += dR;

		float4 ret = mul(float4(currentPos, 1.0f), viewProj);
		float3 screenPos = ret.xyz / ret.w;
		screenPos.x = screenPos.x * 0.5f + 0.5f;
		screenPos.y = -screenPos.y * 0.5f + 0.5f;


		if (saturate(screenPos.z) != screenPos.z
			|| saturate(screenPos.x) != screenPos.x
			|| saturate(screenPos.y) != screenPos.y) return 0.0f.xxx;

		float screenDepth = depthMap.Sample(defaultSampler, screenPos.xy).x;

		if(screenDepth == 1.0f) return float3(0, 0, 0);

		// the ray back off screen
		if (screenDepth < screenPos.z)
		{
			bool isIntersect = false;
			float3 intersectPos = BinarySearch(currentPos - dR, currentPos, viewProj, isIntersect);
			if (isIntersect)
			{
				ret = mul(float4(intersectPos, 1.0f), viewProj);
				screenPos = ret.xyz / ret.w;
				screenPos.x = screenPos.x * 0.5f + 0.5f;
				screenPos.y = -screenPos.y * 0.5f + 0.5f;
				return scene.Sample(defaultSampler, screenPos.xy).xyz;
			}
			return  float3(0, 0, 0);
		}
	}

	return 0.0f.xxx; // float3(0, 0, 1);
}


float4 main(PS_INPUT input) : SV_TARGET
{
	float3 position = positionMap.Sample(defaultSampler, input.textCoord).xyz;
	float3 normal = normalize(normalMap.Sample(defaultSampler, input.textCoord).xyz);

	if (normal.y < 0.98f) return float4(0.0f.xxx, 1.0f);

	return float4(SSR(position, normal, viewPoint, viewProjMat, input.textCoord), 1.0f);
}