#include "../Include/Light.hlsli"

struct PS_INPUT
{
	float4 pos			: SV_POSITION;
	float2 textCoord	: TEXTCOORD;

	//frustum corners world position
	float3 position		: POSITION;
};

#define TEST_LIGHT_INDEX 0

#define FACTOR 8.0f

#define HEIGHT 100.0f

//float3 ComputeSkyColor(Light light, float3 position)
//{
//	if (position.y < 0) return 0.0f.xxx;
//
//	float3 L = -light.dir;
//
//	position.y += 150.0f;
//	float3 V = normalize(position - viewPoint);
//	
//	float LdotV = dot(L, V);
//
//	float rayleigh = 3.0 / (16.0 * PI) * (1.0 + LdotV * LdotV);
//
//	float3 pos = V;
//	float3 fsun = L;
//
//	float Br = 0.0025;
//	float Bm = 0.0003;
//	float g = 0.9800;
//	float3 nitrogen = float3(0.650, 0.570, 0.475);
//	float3 Kr = Br / pow(nitrogen, float3(4.0, 4.0, 4.0));
//	float3 Km = Bm / pow(nitrogen, float3(0.84, 0.84, 0.84));
//
//	float3 mie = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / pow(1.0 + g * g - 2.0 * g * LdotV, 1.5)) / (Br + Bm);
//
//	float3 day_extinction = exp(-exp(-((pos.y + fsun.y * 4.0) * (exp(-pos.y * 16.0) + 0.1) / 80.0) / Br) 
//		* (exp(-pos.y * 16.0) + 0.1) * Kr / Br) * exp(-pos.y * exp(-pos.y * 8.0) * 4.0) * exp(-pos.y * 2.0) * 4.0f;
//	float3 night_extinction = (1.0 - exp(fsun.y)).xxx * 0.2;
//	float3 extinction = lerp(day_extinction, night_extinction, (-fsun.y * 0.2 + 0.5).xxx);
//
//	float3 skyColor = saturate(mie * rayleigh * extinction * light.color * light.power);
//
//	return skyColor;
//}

bool RaySphereIntersect(float3 center, float radius2, float3 orig, float3 dir, out float t)
{
	float t0, t1;
	// geometric solution
	float3 L = center - orig;
	float tca = dot(L, dir);
	// if (tca < 0) return false;
	float d2 = dot(L, L) - tca * tca;
	if (d2 > radius2) return false;
	float thc = sqrt(radius2 - d2);
	t0 = tca - thc;
	t1 = tca + thc;

	if (t0 > t1)
	{
		float temp = t0;
		t0 = t1;
		t1 = temp;
	}

	if (t0 < 0) 
	{
		t0 = t1; // if t0 is negative, let's use t1 instead 
		if (t0 < 0) return false; // both t0 and t1 are negative 
	}

	t = t0;

	return true;
}

float3 ComputeSkyColor(Light light, float3 position)
{
	float t = 0;

	float3 L = -light.dir;

	float3 V = normalize(position - viewPoint);

	const float EarthR = 63600;//100;//6360;
	//const float atmR = 6000;//6420;
	float3 rayOriEarthPos = position;
	rayOriEarthPos.y += EarthR;
	//if (RaySphereIntersect(float3(0, 0, 0), atmR * atmR, rayOriEarthPos, V, t))
	//{
	//	float3 intersectPos = rayOriEarthPos + t * V;

	//	if (intersectPos.y < EarthR) return 0.0f.xxx;
	//}
	//else
	//{
	//	return 0.5f.xxx;
	//}

	if (RaySphereIntersect(float3(0, 0, 0), EarthR * EarthR, rayOriEarthPos, V, t))
	{
		return 0.0f.xxx;
	}

	//if (position.y < 0) return 0.0f.xxx;

	float LdotV = dot(L, V);

	float rayleigh = 3.0 / (16.0 * PI) * (1.0 + LdotV * LdotV);

	const float g = 0.99f;

	float3 mie = 3.f / (8.f * PI) * ((1.f - g * g) * (1.f + LdotV * LdotV)) / ((2.f + g * g) 
		* pow(1.f + g * g - 2.f * g * LdotV, 1.5f));

	const float3 betaR = { 0.038, 0.135, 0.331 };
	const float betaM = 0.021;

	float3 skyColor = saturate(((mie - 1.5f) * betaM * 5 + rayleigh * betaR * 23) * light.color * light.power);

	return skyColor;
}

float4 main(PS_INPUT input) : SV_TARGET0
{
	return float4(ComputeSkyColor(lights[TEST_LIGHT_INDEX], input.position), 1.0f);
	//return float4(1,0,0,1);
}