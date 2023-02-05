// Li Hongcheng
// 2023-01-05



struct VertexIn
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 uv : TEXCOORD;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float3 posL : POSITION;
};

VertexOut VS(VertexIn vsIn)
{
    VertexOut vsOut;
    
    vsOut.posL = vsIn.posL;
    
    
    
    
	return vsOut;
}