#include "../../Include/Light.hlsli"

//struct VS_OUTPUT
//{
//    float4 pos			: SV_POSITION;
//    float4 position		: POSITION;
//    float2 textCoord	: TEXTCOORD;
//}

//struct GS_INPUT
//{
//    float4 pos						: WORLD_POSITION;
//};

struct GS_INPUT
{
    float4 pos			: SV_POSITION;
    float4 position		: POSITION;
    float2 textCoord	: TEXTCOORD;
};

struct GS_OUTPUT
{
    float4 pos						: SV_POSITION;
    uint cubeFaceIndex              : SV_ViewportArrayIndex;
};

cbuffer ShadowInfo : register(b0)
{
    uint index;
    float3 padding;
};

//StructuredBuffer<Light>         lights			: register(t0);
//StructuredBuffer<LightShadow>   shadows			: register(t1);

[maxvertexcount(18)]
void main(inout TriangleStream<GS_OUTPUT> output, triangle GS_INPUT input[3])
{
    [unroll] for (uint i = 0; i < 6; i++)
    {
        GS_OUTPUT outp;
        [unroll] for (uint j = 0; j < 3; j++)
        {
            outp.pos = mul(input[j].position, shadows[index].viewProj[i]);
            outp.cubeFaceIndex = i;
            output.Append(outp);
        }
        output.RestartStrip();
    }

}