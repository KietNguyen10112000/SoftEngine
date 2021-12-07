struct PS_INPUT
{
	float4 pos						: SV_POSITION;
	float2 textCoord				: TEXTCOORD;
	uint   viewport                 : SV_ViewportArrayIndex;
};

float main(PS_INPUT input) : SV_Depth
{
	return 1;
}