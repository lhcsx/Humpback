// (c) Li Hongcheng
// 2023-04-18


cbuffer cbSSAO : register(b0)
{
    float4x4 gInvProj;
}


static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};

struct VSOut
{
    float4 posH : SV_POSITION;
    float3 posV : POSITION;
    float2 texC : TEXCOORD0;
};


VSOut VS(uint vid: SV_VertexID)
{
    VSOut o;
    
    o.texC = gTexCoords[vid];
    o.posH = float4(2.0f * o.texC.x - 1.0f, 1.0f - 2.0f * o.texC.y, 0, 1);
    
    float4 posV = mul(o.posH, gInvProj);
    o.posV = posV.xyz / posV.w;

    return o;
}

float4 PS(VSOut i) : SV_Target
{
    float4 result;

    // TODO

    return result;
}