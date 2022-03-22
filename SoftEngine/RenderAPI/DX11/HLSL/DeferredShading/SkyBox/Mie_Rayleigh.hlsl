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

float3 ComputeSkyColor(Light light, float3 position)
{
	if (position.y < 0) return 0.0f.xxx;

	float3 L = -light.dir;

	position.y += 150.0f;
	float3 V = normalize(position - viewPoint);
	
	float LdotV = dot(L, V);

	float rayleigh = 3.0 / (16.0 * PI) * (1.0 + LdotV * LdotV);

	float3 pos = V;
	float3 fsun = L;

	float Br = 0.0025;
	float Bm = 0.0003;
	float g = 0.9800;
	float3 nitrogen = float3(0.650, 0.570, 0.475);
	float3 Kr = Br / pow(nitrogen, float3(4.0, 4.0, 4.0));
	float3 Km = Bm / pow(nitrogen, float3(0.84, 0.84, 0.84));

	float3 mie = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / pow(1.0 + g * g - 2.0 * g * LdotV, 1.5)) / (Br + Bm);

	float3 day_extinction = exp(-exp(-((pos.y + fsun.y * 4.0) * (exp(-pos.y * 16.0) + 0.1) / 80.0) / Br) 
		* (exp(-pos.y * 16.0) + 0.1) * Kr / Br) * exp(-pos.y * exp(-pos.y * 8.0) * 4.0) * exp(-pos.y * 2.0) * 4.0f;
	float3 night_extinction = (1.0 - exp(fsun.y)).xxx * 0.2;
	float3 extinction = lerp(day_extinction, night_extinction, (-fsun.y * 0.2 + 0.5).xxx);

	float3 skyColor = saturate(mie * rayleigh * extinction * light.color * light.power);

	return skyColor;
}

float4 main(PS_INPUT input) : SV_TARGET0
{
	return float4(ComputeSkyColor(lights[TEST_LIGHT_INDEX], input.position), 1.0f);
	//return float4(1,0,0,1);
}