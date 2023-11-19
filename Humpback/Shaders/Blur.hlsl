// (c) Li Hongcheng
// 2023-09-30


cbuffer cbSSAO : register(b0)
{
    float4x4 _Proj;
    float4x4 _InvProj;
    float4x4 _ProjTex;
    float4 _OffsetVecs[14];
    
    float4 _BlurWeights[3];
    float2 _PixelSize;

    float _Radius;
    float _SurfaceEpsilon;
    float _OcclusionFadeStart;
    float _OcclusionFadeEnd;
}

cbuffer cbRootConstants : register(b1)
{
    bool _HorizontalBlur;
};

Texture2D _NormalMap : register(t0);
Texture2D _DepthMap : register(t1);
Texture2D _InputMap : register(t2);


SamplerState _PointClampSampler : register(s0);
SamplerState _LinearClampSampler : register(s1);
SamplerState _DepthSampler : register(s2);
SamplerState _LinearWrapSampler: register(s3);

static const int _BlurRadius = 5;

static const float2 _TexCoordsArray[6] = 
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};

struct VertexOut
{
    float4 posCS : SV_Position;
    float2 uv : TEXCOORD0;
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut o;

    o.uv = _TexCoordsArray[vid];
    o.posCS = float4(2.0 * o.uv.x - 1.0, 1.0 - 2.0 * o.uv.y, 0.0, 1.0f);

    return o;
}

float NDC2LinearDepth(float ndcDepth)
{
    // ndcDepth = A + B / linearDepth, where A = _ViewToProj[2][2], B = _ViewToProj[3][2].
    return _Proj[3][2] / (ndcDepth - _Proj[2][2]);
}


float4 PS(VertexOut pin) : SV_Target
{
    float blurWeights[12] = {
        _BlurWeights[0].x, _BlurWeights[0].y, _BlurWeights[0].z, _BlurWeights[0].w,
        _BlurWeights[1].x, _BlurWeights[1].y, _BlurWeights[1].z, _BlurWeights[1].w,
        _BlurWeights[2].x, _BlurWeights[2].y, _BlurWeights[2].z, _BlurWeights[2].w
    };

    float2 texOffset;
    if(_HorizontalBlur)
    {
        texOffset = float2(_PixelSize.x, 0.0f);
    }
    else
    {
        texOffset = float2(0.0f, _PixelSize.y);
    }

    float4 color = blurWeights[_BlurRadius] * _InputMap.SampleLevel(_PointClampSampler, pin.uv, 0.0);
    float totalWeights = blurWeights[_BlurRadius];

    float3 centerNormal = _NormalMap.SampleLevel(_PointClampSampler, pin.uv, 0.0f).xyz;
    float depth = _DepthMap.SampleLevel(_DepthSampler, pin.uv, 0.0f).r;
    float linearDepth = NDC2LinearDepth(depth);

    for(float i = -_BlurRadius; i <= _BlurRadius; ++i)
    {
        if(i == 0)
        {
            continue;
        }

        float2 tex = pin.uv + i * texOffset;

        float3 neighborNormal = _NormalMap.SampleLevel(_PointClampSampler, tex, 0.0f).xyz;
        float neighborLinearDepth = NDC2LinearDepth(
            _DepthMap.SampleLevel(_DepthSampler, tex, 0.0).r
        );

        // Discard samples if they are deffer too much in normal and depth.
        if(dot(neighborNormal, centerNormal) >= 0.8 && abs(neighborLinearDepth - linearDepth) <= 0.2)
        {
            float weight = blurWeights[i + _BlurRadius];
            color += weight * _InputMap.SampleLevel(_PointClampSampler, tex, 0.0);
            totalWeights += weight;
        }
    }

    return color / totalWeights;
}