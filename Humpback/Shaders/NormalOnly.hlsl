// (c) Li Hongcheng
// 2023-10-02


#include "Common.hlsl"


struct VertexIn
{
    float3 pos : POSITIONT;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangentU : TANGENT;
};

struct VertexOut
{
    float4 posCS : SV_Position;
    float3 normalWS : NORMAL;
    float3 tangentWS : TANGENT;
    float2 uv : TEXCOORD0;
};

VertexOut VS(VertexIn i)
{
    VertexOut o;
    
    o.posCS = mul(float4(i.pos, 1.0), _World);
    o.posCS = mul(o.posCS, _ViewProj);
    
    o.uv = i.uv;
    
    o.normalWS = mul(i.normal, (float3x3) _World);
    o.tangentWS = mul(i.tangentU, (float3x3) _World);
    
    return o;
}

float4 PS(VertexOut i) : SV_Target
{
    i.normalWS = normalize(i.normalWS);
    float3 normalVS = mul(i.normalWS, (float3x3) _View);
    
    return float4(normalVS, 0.0f);
}