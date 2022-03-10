#define MAX_LIGHT						100000


#define LIGHT_DIR						0
#define LIGHT_POINT						1
#define LIGHT_SPOT						2
#define LIGHT_CSM_DIR					3

#define UINT32_MAX						0xFFFFFFFFu
#define INT32_MAX						0x7FFFFFFF

#define SHADOW_MAP_SIZE					4096.0f
#define SHADOW_MAP_TEXEL_SIZE			(1 / SHADOW_MAP_SIZE)

#define SHADOW_MAP_NUM_CASCADE			4

#define PI								3.14159265359f

//#define DEPTH_BIAS						0.00005f
#define DEPTH_BIAS						envDepthBias

//0.000200f
#define CSM_DEPTH_BIAS					envDepthBias

#define PBR_LIT_FACTOR					10


#define SHADOW_HARDWARE_PCF					1
//#define SHADOW_HARDWARE_FILTERING				1
//#define SHADOW_RAW						1
//#define SHADOW_PCF						1

#define SHADOW_PCF_BEGIN				-1.5f
#define SHADOW_PCF_END					1.5f
#define SHADOW_PCF_TOTAL				9


//#define USE_PIXEL_LIGHT_OFFSET 1

struct Light
{
	float spotAngle;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
	float power;
	
	float3 pos;
	float3 dir;
	float3 color;

	uint type;
	uint index; //index in structured buffer
	uint activeIndex;
	uint activeShadowIndex;
};

struct LightShadow
{
	float2 uvOffset[6];
	float2 texelDim;
	row_major float4x4 viewProj[6];
};

cbuffer Camera : register(b0)
{
	float3 viewPoint;
	float _CameraPadding1;

	//row_major float4x4 viewProjMat;
	//row_major float4x4 invViewProjMat;
	float4x4 viewProjMat;
	float4x4 invViewProjMat;
};

cbuffer LightSystemInfo : register(b1)
{
	float3 environmentAmbient;
	unsigned int numberLight;

	float3 offsetPixelLightFactor;
	float envDepthBias;
};

struct NormalShadingPixel
{
	float3 pixelPos;
	float3 normal;
	float  specular;
	float  shininess;

	float4 color;

	float depth;
};

struct PBRShadingPixel
{
	float3 pos;
	float3 normal;
	float  metallic;
	float  roughness;
	float  ao;

	float4 color;

	float depth;
};

struct LightingResult
{
	float3 diffuse;
	float3 specular;
};

struct ShadingResult
{
	float3 color;
	float3 shadowColor;
};

SamplerState defaultSampler						: register(s0);
SamplerComparisonState shadowMapSampler			: register(s1);

StructuredBuffer<Light>         lights			: register(t0);
StructuredBuffer<LightShadow>   shadows			: register(t1);

Texture2D shadowDepthMap						: register(t2);
Texture2D depthMap								: register(t3);

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

LightingResult DoPointLight(Light light, NormalShadingPixel input)
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

LightingResult DoDirectionalLight(Light light, NormalShadingPixel input)
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

LightingResult DoSpotLight(Light light, NormalShadingPixel input)
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

float3 OffsetPixelLight(in Light light, in float3 pixelPos, in float3 normal)
{
	float3 pToL = light.pos - pixelPos;
	float lenPtoL = length(pToL);

	float3 dir1 = pToL / lenPtoL;

	float cosAlpha = dot(normal, dir1);
	float lenN = cosAlpha * lenPtoL;

	float3 surfaceL = normalize(pToL - lenN * normal);

	return -surfaceL * (
		offsetPixelLightFactor.x + lenPtoL * offsetPixelLightFactor.y + lenPtoL * lenPtoL * offsetPixelLightFactor.z
		);
}


//for dir and spot light
float DoDirSpotLightShadow(in Light light, in LightShadow shadow, float3 pixelPos, in float3 normal)
{
#ifdef USE_PIXEL_LIGHT_OFFSET
	pixelPos += OffsetPixelLight(light, pixelPos, normal);
#endif

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
	/*else
	{
		percentLight = 1;
	}*/

	return percentLight;
}

float DoPointLightShadow(in Light light, in LightShadow shadow, float3 pixelPos, in float3 normal)
{
#ifdef USE_PIXEL_LIGHT_OFFSET
	pixelPos += OffsetPixelLight(light, pixelPos, normal);
#endif

	float3 dir = pixelPos - light.pos;

	uint faceID = CubeFaceID(dir);

	float4 posInLightVP = mul(float4(pixelPos, 1.0f), shadow.viewProj[faceID]);

	float pixelDepth = (posInLightVP.z / posInLightVP.w) - DEPTH_BIAS;

	/*float dx = ddx(pixelDepth);
	float dy = ddy(pixelDepth);
	float bias = max(abs(dx), abs(dy)) * 5;
	pixelDepth = pixelDepth - bias;*/

	float percentLight = 0;

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

inline float CalPercentLight(in float pixelDepth, in float x, in float y)
{
	float percentLight = 0;
#ifdef SHADOW_HARDWARE_PCF
	[unroll] for (float xx = SHADOW_PCF_BEGIN; xx < SHADOW_PCF_END; xx += 1.0f)
	{
		[unroll] for (float yy = SHADOW_PCF_BEGIN; yy < SHADOW_PCF_END; yy += 1.0f)
		{
			percentLight += shadowDepthMap.SampleCmpLevelZero(shadowMapSampler,
				float2(x, y) + float2(xx, yy) * SHADOW_MAP_TEXEL_SIZE, pixelDepth);
		}
	}
	percentLight = percentLight / SHADOW_PCF_TOTAL;
#endif

#ifdef SHADOW_HARDWARE_FILTERING
	percentLight = shadowDepthMap.SampleCmpLevelZero(shadowMapSampler, float2(x, y), pixelDepth);
#endif

#ifdef SHADOW_PCF
	[unroll] for (float xx = SHADOW_PCF_BEGIN; xx < SHADOW_PCF_END; xx += 1.0f)
	{
		[unroll] for (float yy = SHADOW_PCF_BEGIN; yy < SHADOW_PCF_END; yy += 1.0f)
		{
			float cdepth = shadowDepthMap.Load(
				int3(floor((float2(x, y) + float2(xx, yy) * SHADOW_MAP_TEXEL_SIZE) * SHADOW_MAP_SIZE), 0)
			);
			if (cdepth > pixelDepth)
			{
				percentLight += 1;
			}
		}
	}
	percentLight = percentLight / SHADOW_PCF_TOTAL;
#endif

#ifdef SHADOW_RAW
	float cdepth = shadowDepthMap.Load(int3(floor(float2(x, y) * SHADOW_MAP_SIZE), 0));
	if (cdepth > pixelDepth)
	{
		percentLight = 1;
	}
#endif

	return percentLight;
}

float CalCSMDirLit(float3 pixelPos, LightShadow shadow, int index)
{
	float4x4 shadowProj = shadow.viewProj[(int)index];
	float2 uvOffset = shadow.uvOffset[(int)index];
	float2 texelDim = shadow.texelDim;

	float4 posInLightVP = mul(float4(pixelPos, 1.0f), shadowProj);

	float pixelDepth = (posInLightVP.z / posInLightVP.w) - CSM_DEPTH_BIAS;


	float percentLight = 1;

	float x = ((posInLightVP.x / posInLightVP.w) * 0.5f + 0.5f);
	float y = (-(posInLightVP.y / posInLightVP.w) * 0.5f + 0.5f);


	if (saturate(pixelDepth) == pixelDepth && saturate(x) == x && saturate(y) == y)
	{
		float sw = SHADOW_MAP_TEXEL_SIZE / texelDim.x;
		float sh = SHADOW_MAP_TEXEL_SIZE / texelDim.y;

		x = x * sw + uvOffset.x;
		y = y * sh + uvOffset.y;

		percentLight = CalPercentLight(pixelDepth, x, y);
	}
	

	return percentLight;
}

//#define SHADOW_MAP_CASCADE_BAND_WIDTH 0.0001f

const static float SHADOW_MAP_CASCADE_BAND_WIDTH[] = {
	0.001f,
	0.0001f,
	0.00001f,
	0.000001f
};

float DoCSMDirLightShadow(in Light light, in LightShadow shadow, float3 pixelPos, float pixelDepthInScreen, in float3 normal)
{
//#ifdef USE_PIXEL_LIGHT_OFFSET
//	pixelPos += OffsetPixelLight(light, pixelPos, normal);
//#endif
	//=====================choose shadow cascade=============================
	//shadow.viewProj[SHADOW_MAP_NUM_CASCADE] as memory to store depthThres
	const float4 depthThres = shadow.viewProj[SHADOW_MAP_NUM_CASCADE][0];
	float4 currentPixelDepth = float4(pixelDepthInScreen, pixelDepthInScreen, pixelDepthInScreen, pixelDepthInScreen);

	float4 comparison = (currentPixelDepth > depthThres);

	float findex = dot(comparison, float4(1, 1, 1, 1));
	int index = min((int)findex, SHADOW_MAP_NUM_CASCADE - 1);
	//=======================================================================

	//return index / 4.0f;

	float3 toLightV = normalize(-light.dir);
	float cosAngle = saturate(1.0f - dot(toLightV, normal));
	float f = index == SHADOW_MAP_NUM_CASCADE - 1 ? 2 : 1;
	float3 scaledNormalOffset = normal * (offsetPixelLightFactor.x * f * cosAngle);
	pixelPos += scaledNormalOffset;

	float percentLight = CalCSMDirLit(pixelPos, shadow, index);

	if (percentLight == 1) return 1;

	/*float dDepth = depthThres[index] - pixelDepthInScreen;
	if (dDepth < SHADOW_MAP_CASCADE_BAND_WIDTH[index])
	{
		int id = index + 1;
		if (id < SHADOW_MAP_NUM_CASCADE)
		{
			float percentLight2 = CalCSMDirLit(pixelPos, shadow, id);
			float s = dDepth / SHADOW_MAP_CASCADE_BAND_WIDTH[index];
			percentLight = lerp(percentLight2, percentLight, s);
		}
	}*/

	//cascades blending
	if (index != 0)
	{
		int id = index - 1;
		float dDepth = pixelDepthInScreen - depthThres[id];
		if (dDepth < SHADOW_MAP_CASCADE_BAND_WIDTH[id])
		{
			float percentLight2 = CalCSMDirLit(pixelPos, shadow, id);
			float s = dDepth / SHADOW_MAP_CASCADE_BAND_WIDTH[id];
			percentLight = lerp(percentLight2, percentLight, s);
		}
	}

	return percentLight;
}

//N = normal
//L = -lightDir (light = pixelPos - lightPos)
//H = halfWayDir
//V = -viewDir (viewDir = pixelPos - viewPos)
float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / max(denom, 0.0000001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float3 PBRPointLight(Light light, PBRShadingPixel pixel)
{
	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	//F0 = lerp(pixel.metallic, pixel.color.xyz, F0);
	F0 = lerp(F0, pixel.color.xyz, pixel.metallic);

	float3 V = normalize(viewPoint - pixel.pos);

	float3 N = pixel.normal;

	float3 L = light.pos - pixel.pos;
	float distance = length(L);
	L = L / distance;

	float3 H = normalize(V + L);

	float attenuation = DoAttenuation(light, distance);

	float3 radiance = light.color * light.power * PBR_LIT_FACTOR * attenuation;

	// Cook-Torrance BRDF
	float NDF = DistributionGGX(N, H, pixel.roughness);
	float G = GeometrySmith(N, V, L, pixel.roughness);
	float3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

	float3 numerator = NDF * G * F;
	float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	float3 specular = numerator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0

	// kS is equal to Fresnel
	float3 kS = F;
	float3 kD = float3(1, 1, 1) - kS;
	kD *= 1.0 - pixel.metallic;

	float NdotL = max(dot(N, L), 0.0);

	return ((pixel.color.xyz * kD / PI) + specular) * radiance * NdotL;
}

float3 PBRSpotLight(Light light, PBRShadingPixel pixel)
{
	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	//F0 = lerp(pixel.metallic, pixel.color.xyz, F0);
	F0 = lerp(F0, pixel.color.xyz, pixel.metallic);

	float3 V = normalize(viewPoint - pixel.pos);

	float3 N = pixel.normal;

	float3 L = light.pos - pixel.pos;
	float distance = length(L);
	L = L / distance;

	float spotIntensity = DoSpotCone(light, L);

	if (spotIntensity == 0) return float3(0, 0, 0);

	float3 H = normalize(V + L);

	float attenuation = DoAttenuation(light, distance);

	float3 radiance = light.color * light.power * PBR_LIT_FACTOR * attenuation;

	// Cook-Torrance BRDF
	float NDF = DistributionGGX(N, H, pixel.roughness);
	float G = GeometrySmith(N, V, L, pixel.roughness);
	float3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

	float3 numerator = NDF * G * F;
	float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	float3 specular = numerator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0

	// kS is equal to Fresnel
	float3 kS = F;
	float3 kD = float3(1, 1, 1) - kS;
	kD *= 1.0 - pixel.metallic;

	float NdotL = max(dot(N, L), 0.0);

	return ((pixel.color.xyz * kD / PI) + specular) * radiance * NdotL * spotIntensity;
}


float3 PBRDirLight(Light light, PBRShadingPixel pixel)
{
	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	//F0 = lerp(pixel.metallic, pixel.color.xyz, F0);
	F0 = lerp(F0, pixel.color.xyz, pixel.metallic);

	float3 V = normalize(viewPoint - pixel.pos);

	float3 N = pixel.normal;

	float3 L = -light.dir;
	float distance = length(L);
	L = L / distance;

	float3 H = normalize(V + L);

	//float attenuation = DoAttenuation(light, distance);

	float3 radiance = light.color * light.power * PBR_LIT_FACTOR;

	// Cook-Torrance BRDF
	float NDF = DistributionGGX(N, H, pixel.roughness);
	float G = GeometrySmith(N, V, L, pixel.roughness);
	float3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

	float3 numerator = NDF * G * F;
	float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	float3 specular = numerator / max(denominator, 0.001);

	float3 kS = F;
	float3 kD = float3(1, 1, 1) - kS;
	kD *= 1.0 - pixel.metallic;

	float NdotL = max(dot(N, L), 0.0);

	return ((pixel.color.xyz * kD / PI) + specular) * radiance * NdotL;
}


float4 DoNormalShading(NormalShadingPixel pixel)
{
	float percentLight = 1;
	//uint shadowCount = 0;

	//LightingResult lastResult = (LightingResult)0;

	float3 color = 0.0f.xxx;

	[loop] for (unsigned int i = 0; i < numberLight; i++)
	{
		LightingResult result = (LightingResult)0;
		percentLight = 1;
		[branch] switch (lights[i].type)
		{
		case LIGHT_DIR:
		{
			result = DoDirectionalLight(lights[i], pixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoDirSpotLightShadow(lights[i], shadows[lights[i].activeShadowIndex], 
					pixel.pixelPos, pixel.normal);
				//shadowCount++;
			}
		}
		break;
		case LIGHT_POINT:
		{
			result = DoPointLight(lights[i], pixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoPointLightShadow(lights[i], shadows[lights[i].activeShadowIndex], 
					pixel.pixelPos, pixel.normal);
				//shadowCount++;
			}
		}
		break;
		case LIGHT_SPOT:
		{
			result = DoSpotLight(lights[i], pixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoDirSpotLightShadow(lights[i], shadows[lights[i].activeShadowIndex], 
					pixel.pixelPos, pixel.normal);
				//shadowCount++;
			}
		}
		break;
		case LIGHT_CSM_DIR:
		{
			result = DoDirectionalLight(lights[i], pixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoCSMDirLightShadow(lights[i], shadows[lights[i].activeShadowIndex], 
					pixel.pixelPos, pixel.depth, pixel.normal);
			}
		}
		break;
		}

		//lastResult.diffuse += result.diffuse * lights[i].power * percentLight;
		//lastResult.specular += result.specular * lights[i].power * percentLight;

		color += (result.diffuse + result.specular) * lights[i].power * percentLight;
	}

	float4 pixelColor = float4(saturate(environmentAmbient + color
	/*lastResult.diffuse + lastResult.specular*/), 1.0f) * pixel.color;

	return pixelColor;
}

float4 DoPBRShading(PBRShadingPixel pixel)
{
	float3 Lo = float3(0, 0, 0);
	[loop] for (unsigned int i = 0; i < numberLight; i++)
	{
		float3 Li = float3(0, 0, 0);
		float percentLight = 1;
		[branch] switch (lights[i].type)
		{
		case LIGHT_DIR:
		{
			Li = PBRDirLight(lights[i], pixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoDirSpotLightShadow(lights[i], shadows[lights[i].activeShadowIndex], 
					pixel.pos, pixel.normal);
			}
		}
		break;
		case LIGHT_POINT:
		{
			Li = PBRPointLight(lights[i], pixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoPointLightShadow(lights[i], shadows[lights[i].activeShadowIndex], 
					pixel.pos, pixel.normal);
			}
		}
		break;
		case LIGHT_SPOT:
		{
			Li = PBRSpotLight(lights[i], pixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoDirSpotLightShadow(lights[i], shadows[lights[i].activeShadowIndex], 
					pixel.pos, pixel.normal);
			}
		}
		break;
		case LIGHT_CSM_DIR:
		{
			Li = PBRDirLight(lights[i], pixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoCSMDirLightShadow(lights[i], shadows[lights[i].activeShadowIndex], 
					pixel.pos, pixel.depth, pixel.normal);
			}
		}
		break;
		}

		Lo += Li * percentLight;
	}

	float3 ambient = (environmentAmbient) * 0.3 * pixel.color.xyz * pixel.ao;
	float3 color = ambient + Lo;

	color = color / (color + float3(1, 1, 1));

	color = pow(color, float3(1 / 2.2, 1 / 2.2, 1 / 2.2));

	return float4(color, pixel.color.w);
}

ShadingResult DoNormalShading_WithShadowColor(NormalShadingPixel pixel)
{
	float percentLight = 1;

	//float3 shadowColor = 0.0f.xxx;
	float3 color = 0.0f.xxx;
	float3 originColor = 0.0f.xxx;

	[loop] for (unsigned int i = 0; i < numberLight; i++)
	{
		LightingResult result = (LightingResult)0;
		percentLight = 1;
		[branch] switch (lights[i].type)
		{
		case LIGHT_DIR:
		{
			result = DoDirectionalLight(lights[i], pixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoDirSpotLightShadow(lights[i], shadows[lights[i].activeShadowIndex],
					pixel.pixelPos, pixel.normal);
			}
		}
		break;
		case LIGHT_POINT:
		{
			result = DoPointLight(lights[i], pixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoPointLightShadow(lights[i], shadows[lights[i].activeShadowIndex],
					pixel.pixelPos, pixel.normal);
			}
		}
		break;
		case LIGHT_SPOT:
		{
			result = DoSpotLight(lights[i], pixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoDirSpotLightShadow(lights[i], shadows[lights[i].activeShadowIndex],
					pixel.pixelPos, pixel.normal);
			}
		}
		break;
		case LIGHT_CSM_DIR:
		{
			result = DoDirectionalLight(lights[i], pixel);

			if (lights[i].activeShadowIndex != UINT32_MAX)
			{
				percentLight = DoCSMDirLightShadow(lights[i], shadows[lights[i].activeShadowIndex],
					pixel.pixelPos, pixel.depth, pixel.normal);
			}
		}
		break;
		}

		//shadowColor += lights[i].power * (1 - percentLight);
		/*if (percentLight == 1)
			originColor = (result.diffuse + result.specular) * lights[i].power;
		else
			originColor = (result.diffuse) * lights[i].power;*/
		originColor += (result.diffuse + result.specular) * lights[i].power;
		color += originColor * percentLight;
	}

	float3 lightWithShadowColor = saturate(environmentAmbient + color) * pixel.color.xyz;
	float3 lightWithoutShadow = saturate(environmentAmbient + originColor) * pixel.color.xyz;

	ShadingResult ret;
	ret.color = lightWithoutShadow;
	ret.shadowColor = lightWithShadowColor / lightWithoutShadow;

	/*if (any(lightWithoutShadow != lightWithShadowColor))
	{
		ret.color = 0.0f.xxx;
		ret.shadowColor = lightWithShadowColor;
	}
	else
	{
		ret.color = lightWithoutShadow;
		ret.shadowColor = 0.0f.xxx;
	}*/
	

	return ret;
}