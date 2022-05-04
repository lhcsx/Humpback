// (c) Li Hongcheng
// 2021/10/28


struct PSInput
{
    float4 position : SV_POSITION;
    //float2 uv : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION)
{
    PSInput result;

    result.position = position;
    //result.uv = uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
     return float4(input.uv, 1.0, 1.0);

    //return g_texture.Sample(g_sampler, input.uv);
}