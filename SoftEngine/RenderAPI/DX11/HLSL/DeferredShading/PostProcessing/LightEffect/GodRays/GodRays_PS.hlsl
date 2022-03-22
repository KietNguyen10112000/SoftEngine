#include "../../../Include/Light.hlsli"

struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

Texture2D position						: register(t4);

#define NUM_SAMPLES 15

#define G_SCATTERING 0.2f

#define MAX_RAY_LENGTH 600.0f

const static float ditherPattern[4][4] = { 
	{ 0.0f, 0.5f, 0.125f, 0.625f},
	{ 0.75f, 0.22f, 0.875f, 0.375f},
	{ 0.1875f, 0.6875f, 0.0625f, 0.5625},
	{ 0.9375f, 0.4375f, 0.8125f, 0.3125} 
};

//const static float screenWidth = 1280;
//const static float screenHeight = 960;

cbuffer Info : register(b2)
{
	uint  type;
	float screenWidth;
	float screenHeight;
	float padding;
};

float ShadowPointLight(in Light light, in LightShadow shadow, float3 pixelPos)
{
	float3 dir = normalize(pixelPos - light.pos);

	uint faceID = CubeFaceID(dir);

	float4 posInLightVP = mul(float4(pixelPos, 1.0f), shadow.viewProj[faceID]);

	float pixelDepth = (posInLightVP.z / posInLightVP.w) - DEPTH_BIAS;


	float percentLight = 0;

	if (saturate(pixelDepth) == pixelDepth)
	{
		float sw = SHADOW_MAP_TEXEL_SIZE / shadow.texelDim.x;
		float sh = SHADOW_MAP_TEXEL_SIZE / shadow.texelDim.y;

		//2d array
		float x = ((posInLightVP.x / posInLightVP.w) * 0.5f + 0.5f) * sw + shadow.uvOffset[faceID].x;
		float y = (-(posInLightVP.y / posInLightVP.w) * 0.5f + 0.5f) * sh + shadow.uvOffset[faceID].y;

		percentLight = shadowDepthMap.SampleCmpLevelZero(shadowMapSampler,
			float2(x, y), pixelDepth);
	}

	return percentLight;
}

float ShadowSpotLight(in Light light, in LightShadow shadow, float3 pixelPos)
{
	float percentLight = 0;

	float4 posInLightVP = mul(float4(pixelPos, 1.0f), shadow.viewProj[0]);

	float pixelDepth = (posInLightVP.z / posInLightVP.w) - DEPTH_BIAS;

	float x = ((posInLightVP.x / posInLightVP.w) * 0.5f + 0.5f);
	float y = (-(posInLightVP.y / posInLightVP.w) * 0.5f + 0.5f);


	if (saturate(pixelDepth) == pixelDepth && saturate(x) == x && saturate(y) == y)
	{
		float sw = SHADOW_MAP_TEXEL_SIZE / shadow.texelDim.x;
		float sh = SHADOW_MAP_TEXEL_SIZE / shadow.texelDim.y;

		x = x * sw + shadow.uvOffset[0].x;
		y = y * sh + shadow.uvOffset[0].y;

		percentLight = shadowDepthMap.SampleCmpLevelZero(shadowMapSampler, float2(x, y), pixelDepth);
	}

	return percentLight;
}


float ShadowCSMDirLight(in Light light, in LightShadow shadow, float3 pixelPos, float pixelDepthInScreen)
{
	//=====================choose shadow cascade=============================
	//shadow.viewProj[SHADOW_MAP_NUM_CASCADE] as memory to store depthThres
	const float4 depthThres = shadow.viewProj[SHADOW_MAP_NUM_CASCADE][0];
	float4 currentPixelDepth = float4(pixelDepthInScreen, pixelDepthInScreen, pixelDepthInScreen, pixelDepthInScreen);

	float4 comparison = (currentPixelDepth > depthThres);

	float findex = dot(comparison, float4(1, 1, 1, 1));
	int index = min((int)findex, SHADOW_MAP_NUM_CASCADE - 1);
	//=======================================================================



	float4x4 shadowProj = shadow.viewProj[(int)index];
	float2 uvOffset = shadow.uvOffset[(int)index];
	float2 texelDim = shadow.texelDim;

	float4 posInLightVP = mul(float4(pixelPos, 1.0f), shadowProj);

	float pixelDepth = (posInLightVP.z / posInLightVP.w) - DEPTH_BIAS;

	float percentLight = 0;

	float x = ((posInLightVP.x / posInLightVP.w) * 0.5f + 0.5f);
	float y = (-(posInLightVP.y / posInLightVP.w) * 0.5f + 0.5f);

	if (saturate(pixelDepth) == pixelDepth && saturate(x) == x && saturate(y) == y)
	{
		float sw = SHADOW_MAP_TEXEL_SIZE / texelDim.x;
		float sh = SHADOW_MAP_TEXEL_SIZE / texelDim.y;

		x = x * sw + uvOffset.x;
		y = y * sh + uvOffset.y;

		percentLight = shadowDepthMap.SampleCmpLevelZero(shadowMapSampler,
			float2(x, y), pixelDepth);
	}

	return percentLight;
}

float ComputeScattering(float lightDotView, float gscattering)
{
	float result = 1.0f - gscattering * gscattering;
	result /= (4.0f * PI * pow(1.0f + gscattering * gscattering - (2.0f * gscattering) * lightDotView, 1.5f));
	return result;
}

//all pos in world space
float3 GodRaysPointLight(in Light light, float3 pixelPos, in float3 viewPos, float2 uv, float pixelDepthInScreen)
{
	if (pixelDepthInScreen > 0.9999f)
	{
		float2 NDC = float2(uv.x, 1 - uv.y);
		NDC = NDC * 2.0f - 1.0f;
		float4 worldPos = mul(invViewProjMat, float4(NDC, 0.9999f, 1));

		pixelPos = worldPos.xyz / worldPos.w;
	}

	float3 ray = pixelPos - viewPos;

	float3 dir = normalize(ray);
	float rayLength = min(MAX_RAY_LENGTH, length(ray));

	float dL = rayLength / (float)NUM_SAMPLES;

	float3 step = dir * dL;

	int x = floor(uv.x * screenWidth);
	int y = floor(uv.y * screenHeight);

	float ditherValue = ditherPattern[x % 4][y % 4];

	float3 pos = viewPos + ditherValue * step;

	float accumFog = 0;
	for (uint i = 0; i < NUM_SAMPLES; i++)
	{
		float percentLight = ShadowPointLight(light, shadows[light.activeShadowIndex], pos);

		if (percentLight != 0)
		{
			float distance = length(light.pos - pos);
			float attenuation = DoAttenuation(light, distance);

			accumFog += attenuation;
		}
		/*else
		{
			accumFog *= 0.5f;
		}*/
		
		//accumFog += percentLight * attenuation;

		pos += step;
	}

	accumFog = max(0, accumFog);
	accumFog /= (float)NUM_SAMPLES;

	return accumFog * light.power * light.color;
}

float3 GodRaysSpotLight(in Light light, float3 pixelPos, in float3 viewPos, float2 uv, float pixelDepthInScreen)
{
	if (pixelDepthInScreen > 0.9999f)
	{
		float2 NDC = float2(uv.x, 1 - uv.y);
		NDC = NDC * 2.0f - 1.0f;
		float4 worldPos = mul(invViewProjMat, float4(NDC, 0.9999f, 1));

		pixelPos = worldPos.xyz / worldPos.w;
	}

	float3 ray = pixelPos - viewPos;

	float3 dir = normalize(ray);
	float rayLength = min(MAX_RAY_LENGTH, length(ray));

	float dL = rayLength / (float)NUM_SAMPLES;

	float3 step = dir * dL;

	int x = floor(uv.x * screenWidth);
	int y = floor(uv.y * screenHeight);

	float ditherValue = ditherPattern[x % 4][y % 4];

	float3 pos = viewPos + ditherValue * step;

	float accumFog = 0;
	for (uint i = 0; i < NUM_SAMPLES; i++)
	{
		float percentLight = ShadowSpotLight(light, shadows[light.activeShadowIndex], pos);

		if (percentLight != 0)
		{
			float3 L = light.pos - pos;
			float distance = length(L);

			float3 nL = L / distance;

			/*float cosA = dot(nL, light.dir);
			float angle = acos(cosA);

			if (angle < light.spotAngle / 2.0f)
			{*/
			float attenuation = DoAttenuation(light, distance);

			float spotIntensity = DoSpotCone(light, nL);

			accumFog += attenuation * spotIntensity;
			//}
			
		}

		pos += step;
	}

	accumFog = max(0, accumFog);
	accumFog /= (float)NUM_SAMPLES;

	return accumFog * light.power * light.color;
}

float3 GodRaysCSMDirLight(in Light light, float3 pixelPos, in float3 viewPos, float2 uv, float pixelDepthInScreen)
{
	if (pixelDepthInScreen > 0.9999f)
	{
		float2 NDC = float2(uv.x, 1 - uv.y);
		NDC = NDC * 2.0f - 1.0f;
		float4 worldPos = mul(invViewProjMat, float4(NDC, 0.9999f, 1));

		pixelPos = worldPos.xyz / worldPos.w;
	}

	float3 ray = pixelPos - viewPos;

	float3 dir = normalize(ray);
	float rayLength = min(MAX_RAY_LENGTH, length(ray));
	//rayLength = rayLength == 0 ? MAX_RAY_LENGTH : rayLength;

	float dL = rayLength / (float)NUM_SAMPLES;

	float3 step = dir * dL;

	int x = floor(uv.x * screenWidth);
	int y = floor(uv.y * screenHeight);

	float ditherValue = ditherPattern[x % 4][y % 4];

	float3 pos = viewPos + ditherValue * step;

	float sFactor = min(ComputeScattering(dot(dir, -light.dir), light.constantAttenuation) + light.linearAttenuation, 1.0f);

	float accumFog = 0;
	for (uint i = 0; i < NUM_SAMPLES; i++)
	{
		float percentLight = ShadowCSMDirLight(light, shadows[light.activeShadowIndex], pos, pixelDepthInScreen);
        if (percentLight != 0)
		{
            accumFog += sFactor;
        }
		pos += step;
	}

	accumFog = max(0, accumFog);
	accumFog /= (float)NUM_SAMPLES;

	return accumFog * light.power * light.color;
}

float4 main(PS_INPUT input) : SV_TARGET0
{
	float3 pixelPos = position.Sample(defaultSampler, input.textCoord).xyz;

	float pixelDepth = depthMap.Sample(defaultSampler, input.textCoord);

	float3 color = GodRaysCSMDirLight(lights[0], pixelPos, viewPoint, input.textCoord, pixelDepth);

	return float4(color, 1.0f);
}