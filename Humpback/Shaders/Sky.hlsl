// Li Hongcheng
// 2023-01-05


#include "Common.hlsl"


struct VertexIn
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 uv : TEXCOORD;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float3 posL : POSITION;
};

VertexOut VS(VertexIn vsIn)
{
    VertexOut vsOut;
    
    vsOut.posL = vsIn.posL;
    
    float4 posW = mul(float4(vsIn.posL, 1.0f), _World);
    
    posW.xyz += _EyePosW;
    
    vsOut.posH = mul(posW, _ViewProj).xyww;
    
    
	return vsOut;
}

float4 PS(VertexOut psIn) : SV_Target
{
    return _SkyCubeMap.Sample(_SamplerLinearWrap, psIn.posL);
}