// (c) Li Hongcheng
// 2022-12-04


#define MaxLights 16


struct Light
{
	float3 strength;
	float	falloffStart;
	float3 direction;
	float falloffEnd;
	float3 Position;
	float SpotPower;
};

struct Material
{
	float4 diffuseAlbedo;
	float3 fresnelR0;
	float shininess;
};


float4 ComputeLighting(Light gLights[MaxLights], Material mat,
	float3 pos, float3 normal, float3 eyeDir, float3 shadowFactor)
{
	float4 result = 1.0f;





	return result;
}