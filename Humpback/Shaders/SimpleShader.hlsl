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
    float4 result = 1.0;
    pin.normal = normalize(pin.normal);
    
    MaterialData matData = _MaterialDataBuffer[pin.matIdx];
    
    float4 normalSample = _DiffuseMapArray[matData.normalMapIndex].Sample(_SamplerLinearWrap, pin.uv);
    normalSample.xyz = UnpackNormal(normalSample.xyz, pin.normal, pin.tangent);

    float3 eyeDir = normalize(_EyePosW - pin.posW);
    
    float4 diffuse = _DiffuseMapArray[matData.diffuseMapIndex].Sample(_SamplerLinearWrap, pin.uv) * matData.albedo;
    
    
    float shiniess = (1.0f - matData.roughness) * normalSample.a;
    Material mat = { diffuse, matData.fresnelR0, shiniess };
    float shadowFactor = CalShadowFactor(pin.shadowPosCS);
    
    
    float3 directLight = ComputeLighting(lights, mat, pin.posW, normalSample.xyz, eyeDir, shadowFactor);
    
    float2 uvAO = pin.ssaoPosCS / pin.ssaoPosCS.w;
    float ao = _SsaoMap.Sample(_SamplerLinearWrap, uvAO).r;
    float3 ambient = _AmbientLight.rgb * diffuse.rgb * ao;
    
    float3 l = directLight + ambient;
    
    //return float4(ao, ao, ao, 1);
    
    return float4(l, 1);

    return result;
}