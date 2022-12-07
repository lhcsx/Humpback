// (c) Li Hongcheng
// 2021/10/28


#include "Lighting.hlsl"

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

cbuffer matConstants : register(b1)
{
    float4 albedo;
    float3 fresnelR0;
    float roughness;
    float4x4 matTransform;
}

cbuffer cbPass : register(b2)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;
    
    Light lights[MaxLights];
};

struct VertexIn
{
    float3 posL  : POSITION;
    float3 normal : NORMAL;
};

struct VertexOut
{
    float4 posH  : SV_POSITION;
    float3 posW : POSITION;
    float3 normal : NORMAL;
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;

    // Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.posL, 1.0f), gWorld);
    vout.posW = posW;

    vout.posH = mul(posW, gViewProj);

    vout.normal = mul(vin.normal, (float3x3)gWorld);

    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    pin.normal = normalize(pin.normal);

    float3 eyeDir = normalize(gEyePosW - pin.posW);

    float ambient = gAmbientLight * albedo;
    
    float shiniess = 1.0f - roughness;
    Material mat = { albedo, fresnelR0, shiniess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(lights, mat, pin.posW, pin.normal, eyeDir, shadowFactor);
    
    //float4 result = directLight + ambient;
    float4 result = albedo;
    result.a = 1.0;


    return result;
}