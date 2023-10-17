// (c) Li Hongcheng
// 2023-04-18


cbuffer cbSSAO : register(b0)
{
    float4x4 _Proj;
    float4x4 _InvProj;
    float4x4 _ProjTex;
    float4 _OffsetVecs[12];
    
    float _Radius;
    float _SurfaceEpsilon;
    float _OcclusionFadeStart;
    float _OcclusionFadeEnd;
}

Texture2D _NormalMap : register(t0);
Texture2D _DepthTexture : register(t1);
Texture2D _RandomVectorMap : register(t2);

SamplerState _SamplerPointClamp : register(s0);
SamplerState _SamplerDepthClamp : register(s1);
SamplerState _SamplerLinearWrap : register(s2);

static const float2 _TexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};

static const int _SampleCount = 12;

struct VSOut
{
    float4 posH : SV_POSITION;
    float3 posV : POSITION; // On the near plane.
    float2 texC : TEXCOORD0;
};


VSOut VS(uint vid: SV_VertexID)
{
    VSOut o;
    
    o.texC = _TexCoords[vid];
    o.posH = float4(2.0f * o.texC.x - 1.0f, 1.0f - 2.0f * o.texC.y, 0, 1);
    
    float4 posV = mul(o.posH, _InvProj);
    o.posV = posV.xyz / posV.w;

    return o;
}

float NDCDepth2ViewDepth(float ndcDepth)
{
    // depth in ndc equals a + (b / viewZ)
    // a = _hb_invProj[2, 2]
    // b = _hb_invProj[3, 2]
    
    return _Proj[3, 2] / (ndcDepth - _Proj[2, 2]);

}

float EvaluateOcclusion(float zDistance)
{
    float occlusion = 0;
    if(zDistance > _SurfaceEpsilon)
    {
        float fadeLength = _OcclusionFadeEnd - _OcclusionFadeStart;
        occlusion = saturate((_OcclusionFadeEnd - zDistance) / fadeLength);
    }
    
    return occlusion;
}


float4 PS(VSOut i) : SV_Target
{
    float4 result;
    
    float2 screen_uv = i.texC;

    
    float3 n = normalize(_NormalMap.Sample(_SamplerPointClamp, i.texC).xyz);    // View space normal.
    float z = _DepthTexture.Sample(_SamplerDepthClamp, i.texC).r;   // NDC depth.
    z = NDCDepth2ViewDepth(z);
    
    // Conclude view space postion from position on the near plane.
    // pView = t * pNearPlane
    // pView.z = t * pNearPlane.z
    // t = pView.z / pNearPlane.z
    float3 p = (z / i.posV.z) * i.posV;
    
    float3 randomV = _RandomVectorMap.Sample(_SamplerLinearWrap, screen_uv).rgb;
    randomV = 2.0f * randomV - 1.0f;
    
    float occlusionSum = 0.0f;
    for (int i = 0; i < _SampleCount; ++i)
    {
        float3 offset = reflect(_OffsetVecs[i].xyz, randomV);
        float flip = sign(dot(offset, n));
        
        // Sample in the radius.
        float3 samp_vPos = p + flip * _Radius * offset;
        
        float4 samp_texPos = mul(float4(samp_vPos, 1.0f), _ProjTex);
        samp_texPos.xyz /= samp_texPos.w;
        
        float samp_depth = _DepthTexture.Sample(_SamplerDepthClamp, samp_texPos.xy).r;
        samp_depth = NDCDepth2ViewDepth(samp_depth);
        
        // Reconstruct the point on real scene geometry corresponding to the sample point.
        float3 surfact_vPos = (samp_depth / samp_vPos.z) * samp_vPos;
        
        float zDist = p.z - surfact_vPos.z;
        float dp = max(dot(n, normalize(surfact_vPos - p)), 0.0f);
        
        float occlusion = dp * EvaluateOcclusion(zDist);

        occlusionSum += occlusion;
    }
    
    result = occlusionSum / _SampleCount;
    result = 1.0f  - result;
    
    result = saturate(pow(result, 6.0f));
    
    return result;
}