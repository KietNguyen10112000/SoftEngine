#include "../../Include/Light.hlsli"

#define PI 3.14159265359f

struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

cbuffer Camera : register(b0)
{
	float3 viewPoint;
};

cbuffer LightSystemInfo : register(b1)
{
	float3 environmentAmbient;
	unsigned int numberLight;
};

struct Pixel
{
	float3 pixelPos;
	float3 normal;
	float  specular;
	float  shininess;
};

struct LightingResult
{
	float3 diffuse;
	float3 specular;
};

float DoAttenuation(Light light, float d)
{
	return 1.0f / (light.constantAttenuation + light.linearAttenuation * d + light.quadraticAttenuation * d * d);
}

float DoSpotCone(Light light, float3 L)
{
	float minCos = cos(light.spotAngle / 2.0f);
	float maxCos = (minCos + 1.0f) / 2.0f;
	float cosAngle = dot(normalize(light.dir.xyz), -L);
	return smoothstep(minCos, maxCos, cosAngle);
}

float DoSpecular(float3 lightDir, float3 normal, float3 viewDir)
{
	//Phong
	//float3 reflectDir = normalize(reflect(-lightDir, normal));
	//return max(dot(viewDir, reflectDir), 0);

	//Blinn
	float3 halfwayDir = -normalize(lightDir + viewDir);
	return max(dot(halfwayDir, normal), 0);
}

LightingResult DoPointLight(Light light, Pixel input)
{
	LightingResult result;

	float3 lightDir = input.pixelPos - light.pos;

	float distance = length(lightDir);
	float attenuation = DoAttenuation(light, distance);

	lightDir = lightDir / distance;

	float diff = max(0, dot(input.normal, -lightDir));

	float3 viewDir = normalize(input.pixelPos - viewPoint);

	float spec = input.shininess == 0 ? 0 : pow(DoSpecular(lightDir, input.normal, viewDir), input.shininess);

	result.diffuse = diff * attenuation * light.color;
	result.specular = spec * attenuation * light.color * input.specular;

	return result;
}

LightingResult DoDirectionalLight(Light light, Pixel input)
{
	LightingResult result;

	float3 lightDir = normalize(light.dir.xyz);

	float diff = max(0, dot(input.normal, -lightDir));

	float3 viewDir = normalize(input.pixelPos - viewPoint);

	float spec = input.shininess == 0 ? 0 : pow(DoSpecular(lightDir, input.normal, viewDir), input.shininess);

	result.diffuse = diff * light.color;
	result.specular = spec * light.color * input.specular;

	return result;
}

LightingResult DoSpotLight(Light light, Pixel input)
{
	LightingResult result = (LightingResult)0;

	float3 lightDir = input.pixelPos - light.pos;

	float distance = length(lightDir);

	float attenuation = DoAttenuation(light, distance);

	lightDir = lightDir / distance;

	float spotIntensity = DoSpotCone(light, -lightDir);
	/*const float spotIntensity = 1;
	float theta = dot(lightDir, normalize(light.dir));

	if (theta < cos(light.spotAngle / 2.0f))
	{
		return result;
	}*/

	float diff = max(0, dot(input.normal, -lightDir));

	float3 viewDir = normalize(input.pixelPos - viewPoint);

	float spec = input.shininess == 0 ? 0 : pow(DoSpecular(lightDir, input.normal, viewDir), input.shininess);

	result.diffuse = diff * spotIntensity * attenuation * light.color;
	result.specular = spec * spotIntensity * attenuation * light.color * input.specular;

	return result;
}

SamplerState defaultSampler						: register(s0);
SamplerComparisonState shadowMapSampler			: register(s1);

StructuredBuffer<Light>         lights			: register(t0);
StructuredBuffer<LightShadow>   shadows			: register(t1);
Texture2D shadowDepthMap						: register(t2);


Texture2D depthMap								: register(t3);
Texture2D color									: register(t4);
Texture2D positionAndSpec						: register(t5);
Texture2D normAndShininess						: register(t6);




////from https://github.com/Unity-Technologies/Graphics/blob/master/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl
//#define CUBEMAPFACE_POSITIVE_X 0
//#define CUBEMAPFACE_NEGATIVE_X 1
//#define CUBEMAPFACE_POSITIVE_Y 2
//#define CUBEMAPFACE_NEGATIVE_Y 3
//#define CUBEMAPFACE_POSITIVE_Z 4
//#define CUBEMAPFACE_NEGATIVE_Z 5
//
//float CubeMapFaceID(float3 dir)
//{
//	float faceID;
//
//	if (abs(dir.z) >= abs(dir.x) && abs(dir.z) >= abs(dir.y))
//	{
//		faceID = (dir.z < 0.0) ? CUBEMAPFACE_NEGATIVE_Z : CUBEMAPFACE_POSITIVE_Z;
//	}
//	else if (abs(dir.y) >= abs(dir.x))
//	{
//		faceID = (dir.y < 0.0) ? CUBEMAPFACE_NEGATIVE_Y : CUBEMAPFACE_POSITIVE_Y;
//	}
//	else
//	{
//		faceID = (dir.x < 0.0) ? CUBEMAPFACE_NEGATIVE_X : CUBEMAPFACE_POSITIVE_X;
//	}
//
//	return faceID;
//}


uint CubeFaceID(float3 dir)
{
	uint maxIndex = abs(dir.x) > abs(dir.y) ? 0 : 1;
	maxIndex = abs(dir[maxIndex]) > abs(dir.z) ? maxIndex : 2;

	if (dir[maxIndex] < 0)
	{
		maxIndex = 2 * maxIndex + 1;
	}
	else
	{
		maxIndex = 2 * maxIndex;
	}
	return maxIndex;
}


//for dir and spot light
float DoDirSpotLightShadow(Light light, LightShadow shadow, float3 pixelPos)
{
	float percentLight = 0;

	float4 posInLightVP = mul(float4(pixelPos, 1.0f), shadow.viewProj[0]);

	float pixelDepth = posInLightVP.z / posInLightVP.w;

	float x = ((posInLightVP.x / posInLightVP.w) * 0.5f + 0.5f);
	float y = (-(posInLightVP.y / posInLightVP.w) * 0.5f + 0.5f);


	if (saturate(pixelDepth) == pixelDepth && saturate(x) == x && saturate(y) == y)
	{
		float sw = SHADOW_MAP_TEXEL_SIZE / shadow.texelDim.x;
		float sh = SHADOW_MAP_TEXEL_SIZE / shadow.texelDim.y;

		x = x * sw + shadow.uvOffset[0].x;
		y = y * sh + shadow.uvOffset[0].y;

		[unroll] for (float xx = -1.5; xx < 2.5f; xx += 1.0f)
		{
			[unroll] for (float yy = -1.5; yy < 2.5f; yy += 1.0f)
			{
				percentLight += shadowDepthMap.SampleCmpLevelZero(shadowMapSampler,
					float2(x, y) + float2(xx, yy) * SHADOW_MAP_TEXEL_SIZE, pixelDepth);
			}
		}
		percentLight = percentLight / 16.0f;
	}
	else
	{
		percentLight = 1;
	}

	return percentLight;
}

float DoPointLightShadow(Light light, LightShadow shadow, float3 pixelPos)
{
	float3 dir = pixelPos - light.pos;

	uint faceID = CubeFaceID(dir);

	float4 posInLightVP = mul(float4(pixelPos, 1.0f), shadow.viewProj[faceID]);

	float pixelDepth = posInLightVP.z / posInLightVP.w;

	float percentLight = 1;

	if (saturate(pixelDepth) == pixelDepth)
	{
		percentLight = 0;

		float sw = SHADOW_MAP_TEXEL_SIZE / shadow.texelDim.x;
		float sh = SHADOW_MAP_TEXEL_SIZE / shadow.texelDim.y;

		//2d array
		float x = ((posInLightVP.x / posInLightVP.w) * 0.5f + 0.5f) * sw + shadow.uvOffset[faceID].x;
		float y = (-(posInLightVP.y / posInLightVP.w) * 0.5f + 0.5f) * sh + shadow.uvOffset[faceID].y;

		[unroll] for (float xx = -1.5; xx < 2.5f; xx += 1.0f)
		{
			[unroll] for (float yy = -1.5; yy < 2.5f; yy += 1.0f)
			{
				percentLight += shadowDepthMap.SampleCmpLevelZero(shadowMapSampler,
					float2(x, y) + float2(xx, yy) * SHADOW_MAP_TEXEL_SIZE, pixelDepth);
			}
		}
		percentLight = percentLight / 16.0f;

	}

	return percentLight;
}


float4 main(PS_INPUT input) : SV_TARGET
{
	Pixel curPixel = (Pixel)0;

	float pixelDepth = depthMap.Sample(defaultSampler, input.textCoord);

	if (pixelDepth > 0.9999f) return float4(0, 0, 0, 0);

	float4 pixelColor = color.Sample(defaultSampler, input.textCoord);

	//pixel position in world space
	float4 posAndSpec = positionAndSpec.Sample(defaultSampler, input.textCoord);

	curPixel.pixelPos = posAndSpec.xyz;
	curPixel.specular = posAndSpec.w;

	float4 ns = normAndShininess.Sample(defaultSampler, input.textCoord);

	if (all(ns.xyz == 0))
	{
		return pixelColor;
	}

	curPixel.normal = normalize(ns.xyz);
	curPixel.shininess = ns.w;


	float percentLight = 1;
	//uint shadowCount = 0;

	LightingResult lastResult = (LightingResult)0;

	[loop] for (unsigned int i = 0; i < numberLight; i++)
	{
		LightingResult result = (LightingResult)0;
		percentLight = 1;
		[branch] switch (lights[i].type)
		{
		case LIGHT_DIR:
		{
			result = DoDirectionalLight(lights[i], curPixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoDirSpotLightShadow(lights[i], shadows[lights[i].activeShadowIndex], curPixel.pixelPos);
				//shadowCount++;
			}
		}
		break;
		case LIGHT_POINT:
		{
			result = DoPointLight(lights[i], curPixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoPointLightShadow(lights[i], shadows[lights[i].activeShadowIndex], curPixel.pixelPos);
				//shadowCount++;
			}
		}
		break;
		case LIGHT_SPOT:
		{
			result = DoSpotLight(lights[i], curPixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoDirSpotLightShadow(lights[i], shadows[lights[i].activeShadowIndex], curPixel.pixelPos);
				//shadowCount++;
			}
		}
		break;
		}

		lastResult.diffuse += result.diffuse * lights[i].power * percentLight;
		lastResult.specular += result.specular * lights[i].power  * percentLight;
	}

	//percentLight = shadowCount == 0 ? 1 : percentLight / (float)shadowCount;

	/*pixelColor = float4(saturate(environmentAmbient + lastResult.diffuse + lastResult.specular) * percentLight
		+ (1 - percentLight) * pixelColor.xyz * 0.1f, 1.0f) * pixelColor;*/

	pixelColor = float4(saturate(environmentAmbient + lastResult.diffuse + lastResult.specular), 1.0f) * pixelColor;

	return pixelColor;
}