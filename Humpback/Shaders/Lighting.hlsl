// (c) Li Hongcheng
// 2022-12-04


#define MaxLights 16


struct Light
{
	float3 strength;
	float	falloffStart;
	float3 direction;
	float falloffEnd;
	float3 Position;
	float SpotPower;
};

struct Material
{
	float4 diffuseAlbedo;
	float3 fresnelR0;
	float shininess;
};


float3 SchlickFresnel(float3 r0, float3 normal, float3 lightDir)
{
    float cosIncidentAngle = saturate(dot(normal, lightDir));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = r0 + (1.0f - r0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}


float3 BlinnPhong(float3 lightStrength, float3 lightDir, float3 normal, float3 eyeDir, Material mat)
{
    float m = mat.shininess * 256.0f;
    float3 halfVec = normalize(eyeDir + lightDir);
	
    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.fresnelR0, normal, lightDir);

    float3 specAlbedo = fresnelFactor * roughnessFactor;
	
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);
	
    return (mat.diffuseAlbedo.rgb + specAlbedo) * lightStrength;
}


float3 ComputeDirectionLight(Light l, Material mat, float3 normal, float3 eyeDir)
{
    float3 lightDir = -l.direction;
    float ndotl = saturate(dot(normal, lightDir));
    float3 lightStrength = l.strength * ndotl;
    return BlinnPhong(lightStrength, lightDir, normal, eyeDir, mat);
}


float3 ComputeLighting(Light _Lights[MaxLights], Material mat,
	float3 pos, float3 normal, float3 eyeDir, float shadowFactor)
{
	float3 result = 0.0f;

	// Light 0
    Light dirLight = _Lights[0];
    result = shadowFactor * ComputeDirectionLight(dirLight, mat, normal, eyeDir);

	return result;
}