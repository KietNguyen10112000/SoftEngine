#include "../TypeDef.hlsli"

struct PS_INPUT
{
    float4 position		: SV_POSITION;
    float4 color        : COLOR;

    float4 pos          : WORLD_POSITION;
    float4 normal       : WORLD_NORMAL;
};

cbuffer CameraCBuffer : register(b1)
{
    CameraData Camera;
};

const static float3 LIGHT_DIR       = float3(-1, -1, -1);
const static float  LIGHT_POWER     = 1.5f;
//const static float3 LIGHT_AMBIENT   = float3(0.15f, 0.15f, 0.15f);

const static float  LIGHT_AMBIENT   = 0.15f;

float4 main(PS_INPUT input) : SV_TARGET
{
    if (input.color.w == 0.0f)
    {
        return float4(input.color.xyz , 1.0f);
    }

    float3 L = normalize(LIGHT_DIR);
    float3 eye = Camera.transform[3].xyz;
    float3 pos = input.pos.xyz;
    float3 N = normalize(input.normal.xyz);

    float3 V = normalize(pos - eye);

    float diffuse = clamp(dot(N, -L), 0, 1);

    float3 rL = normalize(reflect(L, N));
    float specular = pow(clamp(dot(rL, -V), 0, 1), 128);

    return float4(input.color.xyz * (diffuse + specular) * LIGHT_POWER + input.color.xyz * LIGHT_AMBIENT, 1.0f);
}