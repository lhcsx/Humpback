// Li Hongcheng
// 2023-01-05


#include "Lighting.hlsl"


SamplerState _SamplerPointWrap : register(s0);
SamplerState _SamplerPointClamp : register(s1);
SamplerState _SamplerLinearWrap : register(s2);
SamplerState _SamplerLinearClamp : register(s3);
SamplerState _SamplerAniWrap : register(s4);
SamplerState _SamplerAniClamp : register(s5);
SamplerComparisonState _SamplerShadow : register(s6);

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
    float4x4 _World;
    uint _MatIndex;
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
StructuredBuffer<MaterialData> _MaterialDataBuffer : register(t0, space1);

cbuffer cbPass : register(b1)
{
    float4x4 _View;
    float4x4 _InvView;
    float4x4 _Proj;
    float4x4 _InvProj;
    float4x4 _ViewProj;
    float4x4 _InvViewProj;
    float4x4 _ShadowVPT;
    float3 _EyePosW;
    float _PerObjectPad1;
    float2 _RenderTargetSize;
    float2 _InvRenderTargetSize;
    float _NearZ;
    float _FarZ;
    float _TotalTime;
    float _DeltaTime;
    float4 _AmbientLight;
    
    Light lights[MaxLights];
};

TextureCube _SkyCubeMap : register(t0);
Texture2D _ShadowMap : register(t1);

Texture2D _SsaoMap : register(t2);
Texture2D _DiffuseMapArray[10] : register(t3);


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
    result = mul(result, tbn2World);
    
    return result;
}

float CalShadowFactor(float4 posH)
{
    posH.xyz /= posH.w;
    
    float depth = posH.z;
    uint width, height, mipsCount;
    _ShadowMap.GetDimensions(0, width, height, mipsCount);
    
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
        shadowFactor += _ShadowMap.SampleCmpLevelZero(_SamplerShadow, posH.xy + offsets[i], posH.z).r;
    }

    return shadowFactor / 9.0f;
}