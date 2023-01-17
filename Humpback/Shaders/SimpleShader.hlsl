// (c) Li Hongcheng
// 2021/10/28


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

struct InstanceData
{
    float4x4 world;
    uint materialIndex;
    uint pad0;
    uint pad1;
    uint pad2;
};

StructuredBuffer<InstanceData> gInstanceBuffer : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialDataBuffer : register(t1, space1);

Texture2D gDiffuseMapArray[4] : register(t0);

//cbuffer cbPerObject : register(b0)
//{
//    float4x4 gWorld;
//    uint gMatIndex;
//    uint pad0;
//    uint pad1;
//    uint pad2;
//};

cbuffer cbPass : register(b0)
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
    float2 uv : TEXCOORD;
};

struct VertexOut
{
    float4 posH  : SV_POSITION;
    float3 posW : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    
    nointerpolation uint matIdx : MATINDEX;
};

VertexOut VSMain(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout;
    
    InstanceData insData = gInstanceBuffer[instanceID];
    
    // Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.posL, 1.0f), insData.world);
    vout.posW = posW.xyz;

    vout.posH = mul(posW, gViewProj);

    vout.normal = mul(vin.normal, (float3x3)insData.world);
    vout.uv = vin.uv;
    
    vout.matIdx = insData.materialIndex;

    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float4 result = 1.0;
    pin.normal = normalize(pin.normal);
    
    MaterialData matData = gMaterialDataBuffer[pin.matIdx];
    
    float3 eyeDir = normalize(gEyePosW - pin.posW);
    
    float4 diffuse = gDiffuseMapArray[matData.diffuseMapIndex].Sample(samLinearWrap, pin.uv) * matData.albedo;
    
    float3 ambient = gAmbientLight.rgb + diffuse.rgb;
    
    float shiniess = 1.0f - matData.roughness;
    Material mat = { matData.albedo, matData.fresnelR0, shiniess };
    float3 shadowFactor = 1.0f;
    float3 directLight = ComputeLighting(lights, mat, pin.posW, pin.normal, eyeDir, shadowFactor);
    
    float3 l = directLight + ambient;
    result = float4(l * 0.9, matData.albedo.a);

    return result;
}