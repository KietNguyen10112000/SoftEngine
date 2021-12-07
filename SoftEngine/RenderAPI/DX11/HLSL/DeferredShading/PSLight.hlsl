#include "Include/Light.hlsli"

struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;
};

cbuffer Camera : register(b0)
{
	float4 viewPoint;
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
	float minCos = cos(light.spotAngle);
	float maxCos = (minCos + 1.0f) / 2.0f;
	float cosAngle = dot(normalize(-light.dir.xyz), -L);
	return smoothstep(minCos, maxCos, cosAngle);
}

float DoSpecular(float3 lightDir, float3 normal, float3 viewDir)
{
	float3 reflectDir = -reflect(lightDir, normal);
	return max(dot(viewDir, reflectDir), 0);
}

LightingResult DoPointLight(Light light, Pixel input)
{
	LightingResult result;

	float3 lightDir = input.pixelPos - light.pos;

	float distance = length(lightDir);
	float attenuation = DoAttenuation(light, distance);

	lightDir = normalize(lightDir);

	float diff = max(0, dot(input.normal, -lightDir));

	//float3 reflectDir = reflect(-lightDir, input.normal);

	float3 viewDir = normalize(input.pixelPos - viewPoint);

	//float spec = input.specular == 0 ? 0 : pow(max(dot(viewDir, reflectDir), 0), input.specular);

	float spec = input.shininess == 0 ? 0 : pow(DoSpecular(lightDir, input.normal, viewDir), input.shininess);

	result.diffuse = diff * attenuation * light.color;
	result.specular = spec * attenuation * light.color * input.specular;

	return result;
}

LightingResult DoDirectionalLight(Light light, Pixel input)
{
	LightingResult result;

	float3 lightDir = light.dir.xyz;

	float diff = max(0, dot(input.normal, -lightDir));

	//float3 reflectDir = reflect(-lightDir, input.normal);

	float3 viewDir = normalize(input.pixelPos - viewPoint);

	//float spec = input.specular == 0 ? 0 : pow(max(dot(viewDir, reflectDir), 0), input.specular);

	float spec = input.shininess == 0 ? 0 : pow(DoSpecular(lightDir, input.normal, viewDir), input.shininess);

	result.diffuse = diff * light.color;
	result.specular = spec * light.color * input.specular;

	return result;
}

LightingResult DoSpotLight(Light light, Pixel input)
{
	LightingResult result;

	float3 lightDir = input.pixelPos - light.pos;

	float distance = length(lightDir);

	float attenuation = DoAttenuation(light, distance);

	lightDir = normalize(lightDir);
	float spotIntensity = DoSpotCone(light, lightDir);

	float diff = max(0, dot(input.normal, -lightDir));

	//float3 reflectDir = reflect(-lightDir, input.normal);

	float3 viewDir = normalize(input.pixelPos - viewPoint);

	//float spec = input.specular == 0 ? 0 : pow(max(dot(viewDir, reflectDir), 0), input.specular);

	float spec = input.shininess == 0 ? 0 : pow(DoSpecular(lightDir, input.normal, viewDir), input.shininess);

	result.diffuse = diff * spotIntensity * attenuation * light.color;
	result.specular = spec * spotIntensity * attenuation * light.color * input.specular;

	return result;
}

SamplerState defaultSampler				: register(s0);
SamplerComparisonState shadowMapSampler : register(s1);

Texture2D color							: register(t0);
Texture2D positionAndSpec				: register(t1);
Texture2D normAndShininess				: register(t2);

Texture2D depthMap						: register(t4);

Texture2D shadowDepthMap				: register(t5);

//TextureCube shadowDepthCubeMap			: register(t6);
Texture2DArray shadowDepthCubeMap		: register(t6);

StructuredBuffer<Light> lights			: register(t3);


cbuffer LightShadow : register(b3)
{
	row_major float4x4 lightVP;
};


//for dir and spot light
float DoShadow(Light light, Pixel input)
{
	float percentLight = 0;

	float4 posInLightVP = mul(float4(input.pixelPos, 1.0f), lightVP);

	float pixelDepth = posInLightVP.z / posInLightVP.w;

	float x = (posInLightVP.x / posInLightVP.w) * 0.5f + 0.5f;
	float y = -(posInLightVP.y / posInLightVP.w) * 0.5f + 0.5f;

	if (saturate(pixelDepth) == pixelDepth)
	{
		//float3 lightDir = input.pixelPos - light.pos;
		//float bias = clamp(0.001f * (1.0f - dot(input.normal, lightDir)), 0.0001f, 0.001f);

		/*percentLight = shadowDepthMap.SampleCmpLevelZero(shadowMapSampler,
			float2(x, y), pixelDepth);*/

		const float2 depthMapTexelSize = float2(1 / 1280.0f, 1 / 960.0f);

		[unroll] for (float xx = -1.5; xx < 2.5f; xx += 1.0f)
		{
			[unroll] for (float yy = -1.5; yy < 2.5f; yy += 1.0f)
			{
				percentLight += shadowDepthMap.SampleCmpLevelZero(shadowMapSampler,
					float2(x, y) + float2(xx, yy) * depthMapTexelSize, pixelDepth);
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

cbuffer PointViewProj : register(b4)
{
	row_major float4x4 pointLightVP[6];
};

static const float3 sampleOffsetDirections[20] =
{
	float3(1, 1, 1), float3(1, -1, 1), float3(-1, -1, 1), float3(-1, 1, 1),
	float3(1, 1, -1), float3(1, -1, -1), float3(-1, -1, -1), float3(-1, 1, -1),
	float3(1, 1, 0), float3(1, -1, 0), float3(-1, -1, 0), float3(-1, 1, 0),
	float3(1, 0, 1), float3(-1, 0, 1), float3(1, 0, -1), float3(-1, 0, -1),
	float3(0, 1, 1), float3(0, -1, 1), float3(0, -1, -1), float3(0, 1, -1)
};

static const uint numsamples = 20;

float DoShadowPointLight(Light light, Pixel input)
{
	float3 dir = input.pixelPos - light.pos;

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

	float4 posInLightVP = mul(float4(input.pixelPos, 1.0f), pointLightVP[maxIndex]);

	float pixelDepth = posInLightVP.z / posInLightVP.w;

	//float x = (posInLightVP.x / posInLightVP.w) * 0.5f + 0.5f;
	//float y = -(posInLightVP.y / posInLightVP.w) * 0.5f + 0.5f;

	float percentLight = 1;

	if (saturate(pixelDepth) == pixelDepth)
	{
		percentLight = 0;

		/*percentLight = shadowDepthCubeMap.SampleCmpLevelZero(shadowMapSampler,
			dir, pixelDepth);

		float viewDistance = length(viewPoint - input.pixelPos);
		float diskRadius = (1.0f + (viewDistance / 250.0f)) / 25.0f;

		[unroll] for (uint i = 0; i < numsamples; i++)
		{
			percentLight += shadowDepthCubeMap.SampleCmpLevelZero(shadowMapSampler,
				dir + sampleOffsetDirections[i] * diskRadius, pixelDepth);
		}
		percentLight /= (float)numsamples + 1.f;*/

		/*percentLight = 0;
		for (uint i = 0; i < numsamples; i++)
		{
			float closetDepth = shadowDepthCubeMap.Sample(defaultSampler, dir + sampleOffsetDirections[i] * 0.05f);
			percentLight += closetDepth < pixelDepth ? 0 : 1;
		}
		percentLight /= (float)numsamples;*/

		//const float samples = 3.0f;
		//const float offset = 0.05f;
		/*const float samples = 4.0f;
		const float offset = 0.1f;
		[unroll] for (float x = -offset; x < offset; x += offset / (samples * 0.5))
		{
			[unroll] for (float y = -offset; y < offset; y += offset / (samples * 0.5))
			{
				[unroll] for (float z = -offset; z < offset; z += offset / (samples * 0.5))
				{
					percentLight += shadowDepthCubeMap.SampleCmpLevelZero(shadowMapSampler,
						dir + float3(x, y, z), pixelDepth);
				}
			}
		}
		percentLight /= (samples * samples * samples);*/


		//2d array
		float x = (posInLightVP.x / posInLightVP.w) * 0.5f + 0.5f;
		float y = -(posInLightVP.y / posInLightVP.w) * 0.5f + 0.5f;

		//percentLight = shadowDepthCubeMap.SampleCmpLevelZero(shadowMapSampler,
			//float3(x, y, maxIndex), pixelDepth);

		const float2 depthMapTexelSize = float2(1 / 960.0f, 1 / 960.0f);

		[unroll] for (float xx = -1.5; xx < 2.5f; xx += 1.0f)
		{
			[unroll] for (float yy = -1.5; yy < 2.5f; yy += 1.0f)
			{
				percentLight += shadowDepthCubeMap.SampleCmpLevelZero(shadowMapSampler,
					float3(float2(x, y) + float2(xx, yy) * depthMapTexelSize, maxIndex), pixelDepth);
			}
		}
		percentLight = percentLight / 16.0f;

	}

	return percentLight;
}

//float DoShadowPointLight_v2(Light light, Pixel input, float farP)
//{
//	float3 dir = input.pixelPos - light.pos;
//
//	float pixelDepth = length(dir);
//
//	pixelDepth /= farP;
//
//	float percentLight = 1;
//
//	if (saturate(pixelDepth) == pixelDepth)
//	{
//		percentLight = shadowDepthCubeMap.SampleCmpLevelZero(shadowMapSampler,
//			dir, pixelDepth);
//		//percentLight = closetDepth < pixelDepth ? 0 : 1;
//	}
//
//	return percentLight;
//}


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

	curPixel.normal = ns.xyz;
	curPixel.shininess = ns.w;

	if (all(curPixel.normal == 0))
	{
		return pixelColor;
	}

	//float percentLight = DoShadow(lights[0], curPixel);

	float percentLight1 = DoShadowPointLight(lights[0], curPixel);

	float percentLight2 = DoShadow(lights[1], curPixel);

	float percentLight = (percentLight1 + percentLight2) / 2.0f;

	LightingResult lastResult = (LightingResult)0;
	
	[loop] for (unsigned int i = 0; i < numberLight; i++)
	{
		LightingResult result = (LightingResult)0;
		[branch] switch (lights[i].type)
		{
		case LIGHT_DIR:
		{
			result = DoDirectionalLight(lights[i], curPixel);
		}
		break;
		case LIGHT_POINT:
		{
			result = DoPointLight(lights[i], curPixel);
		}
		break;
		case LIGHT_SPOT:
		{
			result = DoSpotLight(lights[i], curPixel);
		}
		break;
		}

		lastResult.diffuse += result.diffuse * lights[i].power;
		lastResult.specular += result.specular * lights[i].power;
	}

	pixelColor = float4(saturate(environmentAmbient + lastResult.diffuse + lastResult.specular) * percentLight 
		+ (1 - percentLight) * pixelColor.xyz * 0.1f, 1.0f) * pixelColor;

	return pixelColor;
}