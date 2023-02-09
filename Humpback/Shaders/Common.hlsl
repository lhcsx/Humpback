// Li Hongcheng
// 2023-01-05


#include "Lighting.hlsl"


SamplerState samPointWrap : register(s0);
SamplerState samPointClamp : register(s1);
SamplerState samLinearWrap : register(s2);
SamplerState samLinearClamp : register(s3);
SamplerState samAniWrap : register(s4);
SamplerState samAniClamp : register(s5);

struct MaterialData
{
    float4 albedo;
    float3 fresnelR0;
    float roughness;
    float4x4 matTransform;
    uint diffuseMapIndex;
    uint pad0;
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
Texture2D gDiffuseMapArray[4] : register(t1); 