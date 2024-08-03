//#define D3DCOMPILE_DEBUG 1
#include "Common/TypeDef.hlsli"

//const static float ditherPattern[8][8] =
//{
//    { 0, 32, 8, 40, 2, 34, 10, 42 },
//    { 48, 16, 56, 24, 50, 18, 58, 26 },
//    { 12, 44, 4, 36, 14, 46, 6, 38 },
//    { 60, 28, 52, 20, 62, 30, 54, 22 },
//    { 3, 35, 11, 43, 1, 33, 9, 41 },
//    { 51, 19, 59, 27, 49, 17, 57, 25 },
//    { 15, 47, 7, 39, 13, 45, 5, 37 },
//    { 63, 31, 55, 23, 61, 29, 53, 21 }
//};

#define DITHER_PATTERN_BASE 16
#define DITHER_PATTERN_BASE2 256
const static float ditherPattern[16][16] =
{
    { 0, 128, 32, 160, 8, 136, 40, 168, 2, 130, 34, 162, 10, 138, 42, 170 },
    { 192, 64, 224, 96, 200, 72, 232, 104, 194, 66, 226, 98, 202, 74, 234, 106 },
    { 48, 176, 16, 144, 56, 184, 24, 152, 50, 178, 18, 146, 58, 186, 26, 154 },
    { 240, 112, 208, 80, 248, 120, 216, 88, 242, 114, 210, 82, 250, 122, 218, 90 },
    { 12, 140, 44, 172, 4, 132, 36, 164, 14, 142, 46, 174, 6, 134, 38, 166 },
    { 204, 76, 236, 108, 196, 68, 228, 100, 206, 78, 238, 110, 198, 70, 230, 102 },
    { 60, 188, 28, 156, 52, 180, 20, 148, 62, 190, 30, 158, 54, 182, 22, 150 },
    { 252, 124, 220, 92, 244, 116, 212, 84, 254, 126, 222, 94, 246, 118, 214, 86 },
    { 3, 131, 35, 163, 11, 139, 43, 171, 1, 129, 33, 161, 9, 137, 41, 169 },
    { 195, 67, 227, 99, 203, 75, 235, 107, 193, 65, 225, 97, 201, 73, 233, 105 },
    { 51, 179, 19, 147, 59, 187, 27, 155, 49, 177, 17, 145, 57, 185, 25, 153 },
    { 243, 115, 211, 83, 251, 123, 219, 91, 241, 113, 209, 81, 249, 121, 217, 89 },
    { 15, 143, 47, 175, 7, 135, 39, 167, 13, 141, 45, 173, 5, 133, 37, 165 },
    { 207, 79, 239, 111, 199, 71, 231, 103, 205, 77, 237, 109, 197, 69, 229, 101 },
    { 63, 191, 31, 159, 55, 183, 23, 151, 61, 189, 29, 157, 53, 181, 21, 149 },
    { 255, 127, 223, 95, 247, 119, 215, 87, 253, 125, 221, 93, 245, 117, 213, 85 }
};

struct PS_INPUT
{
	float4 svposition			: SV_POSITION;
	float3 position				: POSITION;
	float3x3 TBN				: TBN_MATRIX;
    float3 textCoordAndAlpha	: TEXTCOORD;
};

SamplerState	defaultSampler			: register(s0);

Texture2D		color					: register(t0, space1);

cbuffer CameraCBuffer : register(b0, SPACE_PS)
{
	CameraData Camera;
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

float DoSpecular(float3 lightDir, float3 normal, float3 viewDir)
{
	//Phong
	/*float3 reflectDir = normalize(reflect(-lightDir, normal));
	return max(dot(viewDir, reflectDir), 0);*/

	//Blinn
	float3 halfwayDir = -normalize(lightDir + viewDir);
	return max(dot(halfwayDir, normal), 0);
}

LightingResult DoDirectionalLight(Light light, Pixel input, float3 viewPoint)
{
	LightingResult result;

	float3 lightDir = light.dir.xyz;

	float diff = max(0, dot(input.normal, -lightDir));

	float3 viewDir = normalize(input.pixelPos - viewPoint);

	float spec = input.shininess == 0 ? 0 : pow(DoSpecular(lightDir, input.normal, viewDir), input.shininess);

	result.diffuse = diff * light.color;
	result.specular = spec * light.color * input.specular;

	return result;
}

float4 main(PS_INPUT input) : SV_TARGET
{
	Pixel px;
	px.pixelPos = input.position;
	px.normal = normalize(input.TBN[2]);
	px.specular = 0.5f;
	px.shininess = 64;
	
    float alphaChannel = 1.0f;
    if (input.textCoordAndAlpha.z < 0.999f)
    {
        if (input.textCoordAndAlpha.z == 0)
        {
            discard;
        }
		
        int x = floor(input.svposition.x);
        int y = floor(input.svposition.y);
        int ditherValue = int(ditherPattern[x % DITHER_PATTERN_BASE][y % DITHER_PATTERN_BASE]);
		
        int base = floor(input.textCoordAndAlpha.z * DITHER_PATTERN_BASE2);
        if (ditherValue > base)
        {
			discard;
        }
    }

	LightingResult lastResult = DoDirectionalLight(g_light, px, Camera.transform._m30_m31_m32);

    float4 pixelColor = float4(color.Sample(defaultSampler, input.textCoordAndAlpha.xy).rgb, 1.0f);
    pixelColor *= float4(saturate((lastResult.diffuse + lastResult.specular)), alphaChannel);
	
	return pixelColor;

	//return float4(Camera.transform._m30_m31_m32, 1.0f);
}