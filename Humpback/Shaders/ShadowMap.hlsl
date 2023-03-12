// (c) Li Hongcheng
// 2023-03-08


#include "Common.hlsl"

struct VSIN
{
    float3 pos : POSITION;
};


float4 VS(VSIN vsin) : SV_POSITION
{
    float4 posW = mul(float4(vsin.pos, 1.0f), gWorld);
    float4 posH = mul(posW, gViewProj);
    
	return posH;
}