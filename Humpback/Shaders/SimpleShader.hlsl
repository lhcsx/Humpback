// (c) Li Hongcheng
// 2021/10/28


cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
};


struct PSInput
{
    float4 PosH : SV_POSITION;
    float4 Color: COLOR;
};


PSInput VSMain(VertexIn vin)
{
    PSInput result;

    result.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    result.Color = vin.Color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.Color;
}