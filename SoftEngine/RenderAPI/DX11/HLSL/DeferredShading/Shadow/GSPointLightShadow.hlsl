struct GS_INPUT
{
    float4 pos						: WORLD_POSITION;
};

struct GS_OUTPUT
{
    float4 pos						: SV_POSITION;
    //float4 position                 : WORLD_POSITION;
    uint cubeFaceIndex              : SV_RenderTargetArrayIndex;
};

cbuffer PointViewProj : register(b0)
{
    row_major float4x4 vp[6];
};

[maxvertexcount(18)]
void main(inout TriangleStream<GS_OUTPUT> output, triangle GS_INPUT input[3])
{
    [unroll] for (uint i = 0; i < 6; i++)
    {
        GS_OUTPUT outp;
        [unroll] for (uint j = 0; j < 3; j++)
        {
            //outp.position = input[j].pos;
            outp.pos = mul(input[j].pos, vp[i]);
            //outp.position = outp.pos;
            outp.cubeFaceIndex = i;
            output.Append(outp);
        }
        output.RestartStrip();
    }

}