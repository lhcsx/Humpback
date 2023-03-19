// Li Hongcheng
// 2023-01-05


#include "Lighting.hlsl"


SamplerState samPointWrap : register(s0);
SamplerState samPointClamp : register(s1);
SamplerState samLinearWrap : register(s2);
SamplerState samLinearClamp : register(s3);
SamplerState samAniWrap : register(s4);
SamplerState samAniClamp : register(s5);
SamplerComparisonState samShadow : register(s6);

struct MaterialData
{
    float4 albedo;
    float3 fresnelR0;
    float roughness;
    float4x4 matTransform;
    uint diffuseMapIndex;
    uint normalMapIndex;
    uint pad1;
    uint pad2;
};


cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    uint gMatIndex;
    uint pad0;
    uint pad1;
    uint pad2;
};


//struct InstanceData
//{
//    float4x4 world;
//    uint materialIndex;
//    uint pad0;
//    uint pad1;
//    uint pad2;
//};

//StructuredBuffer<InstanceData> gInstanceBuffer : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialDataBuffer : register(t0, space1);

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 gShadowVPT;
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

TextureCube gSkyCubeMap : register(t0);
Texture2D gShadowMap : register(t1);

Texture2D gDiffuseMapArray[4] : register(t2);


float3 UnpackNormal(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
    float3 result = 0;
    
    // Scale normal frome [0, 1] - [-1, 1];
    result = normalMapSample * 2.0f - 1.0f;
    
    float3 N = unitNormalW;
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);
    
    // Transform normal from TBN space to world sapce.
    float3x3 tbn2World = float3x3(T, B, N);
    result = mul(normalMapSample, tbn2World);
    
    return result;
}

float CalShadowFactor(float4 posH)
{
    posH.xyz /= posH.w;
    
    float depth = posH.z;
    uint width, height, mipsCount;
    gShadowMap.GetDimensions(0, width, height, mipsCount);
    
    float dx = 1.0f / width;    // Texel size.
    
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(dx, -dx), float2(dx, dx), float2(0.0f, dx)
    };
    
    float shadowFactor = 0.0f;
    
    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        uint status = 0;
        shadowFactor = gShadowMap.SampleCmpLevelZero(samShadow, posH.xy + offsets[i], posH.z).r;
    }

    return shadowFactor / 9.0f;
}