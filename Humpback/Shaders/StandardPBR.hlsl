// (c) Li Hongcheng
// 2023-11-26


#include "Common.hlsl"


struct VertexIn
{
    float3 posL : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
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

    vout.normal = mul(vin.normal, (float3x3) _World);
    vout.uv = vin.uv;
    
    vout.matIdx = _MatIndex;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    MaterialData matData = _MaterialDataBuffer[pin.matIdx];


    float3 albedo = _DiffuseMapArray[matData.diffuseMapIndex].Sample(_SamplerLinearWrap, pin.uv);
    float4 metallicSmothness = _DiffuseMapArray[matData.metallicSmothnessMapIndex].Sample(_SamplerLinearWrap, pin.uv);
    float smoothness = metallicSmothness.a;
    float metallic = metallicSmothness.r;
    BRDFData brdfData = InitializeBRDFData(albedo, metallic, smoothness);
    Light mainLight = GetMainLight();
    // float shadowFactor = CalShadowFactor(pin.shadowPosCS);
    float shadowFactor = 1;
    // float4 normalSample = _DiffuseMapArray[matData.normalMapIndex].Sample(_SamplerLinearWrap, pin.uv);
    float4 normalSample = _DiffuseMapArray[matData.normalMapIndex].Sample(_SamplerLinearWrap, pin.uv);
    pin.normal = normalize(pin.normal);
    normalSample.xyz = UnpackNormal(normalSample.xyz, pin.normal, pin.tangent);
    float3 eyeDir = normalize(_EyePosW - pin.posW);

    float3 directLight = LightingPhysicallyBased(brdfData, mainLight, shadowFactor, normalSample.xyz, eyeDir);

    float2 uvAO = pin.ssaoPosCS / pin.ssaoPosCS.w;
    // float ao = _SsaoMap.Sample(_SamplerLinearWrap, uvAO).r;
    float ao = 1;

    float3 ambient = _AmbientLight.rgb * albedo.rgb * ao * 2;

    float3 lighting = directLight + ambient;

    return float4(lighting, 1);
}