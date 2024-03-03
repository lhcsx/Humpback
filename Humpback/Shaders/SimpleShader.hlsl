// (c) Li Hongcheng
// 2021/10/28


#include "Common.hlsl"


struct VertexIn
{
    float3 posL  : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

struct VertexOut
{
    float4 posH  : SV_POSITION;
    float4 shadowPosCS : POSITION0;
    float4 ssaoPosCS : POSITION1;
    float3 posW : POSITION2;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
    
    nointerpolation uint matIdx : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout;
    
    // Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.posL, 1.0f), _World);
    vout.posW = posW.xyz;
    vout.posH = mul(posW, _ViewProj);
    vout.shadowPosCS = mul(posW, _ShadowVPT);
    vout.ssaoPosCS = mul(posW, _ViewProjTex);
    
    vout.tangent = mul(float4(vin.tangent, 0.0f), _World).xyz;

    vout.normal = mul(vin.normal, (float3x3)_World);
    vout.uv = vin.uv;
    
    vout.matIdx = _MatIndex;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    MaterialData matData = _MaterialDataBuffer[pin.matIdx];
    return float4(matData.albedo.rgb, 1);
}