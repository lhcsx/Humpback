// (c) Li Hongcheng
// 2023-11-26


struct BRDFData
{
    float3 abledo;
    float3 diffuse;
    float3 specular;
    float roughness;
    float roughness2;

    float roughness2MinusOne;
};