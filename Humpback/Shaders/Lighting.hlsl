// (c) Li Hongcheng
// 2022-12-04


#include "BRDF.hlsl"


#define MaxLights 16

#define kDielectricSpec float4(0.04, 0.04, 0.04, 1.0 - 0.04)


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

float DirectBRDFSpecular(BRDFData brdfData, float3 normalWS, float3 lightDirectionWS, float3 viewDirectionWS)
{
    float3 halfVec = normalize(lightDirectionWS + viewDirectionWS);
    float NoH = saturate(dot(normalWS, halfVec));
    float LoH = saturate(dot(lightDirectionWS, halfVec));

    // GGX Distribution multiplied by combined approximation of Visibility and Fresnel
    // BRDFspec = (D * V * F) / 4.0
    // D = roughness^2 / ( NoH^2 * (roughness^2 - 1) + 1 )^2
    // V * F = 1.0 / ( LoH^2 * (roughness + 0.5) )
    float D = brdfData.roughness2 / pow(( (NoH * NoH * (brdfData.roughness2 - 1.0) + 1.0) ), 2);
    float VF = 1.0 / (LoH * LoH * (brdfData.roughness + 0.5));
    
    float specularTerm = (D * VF) / 4.0;

    return specularTerm;
}

float3 LightingPhysicallyBased(BRDFData brdfData, Light mainLight, float lightAttenuation, float3 normalWS, float3 viewDirectionWS)
{
    float3 lightDirectionWS = -mainLight.direction;
    float3 lightColor = mainLight.strength;

    float NdotL = saturate(dot(normalWS, lightDirectionWS));

    float3 radiance = lightColor * lightAttenuation * NdotL;

    float3 brdf = brdfData.diffuse;

    brdf += brdfData.specular * DirectBRDFSpecular(brdfData, normalWS, lightDirectionWS, viewDirectionWS);

    return brdf * radiance;
}

float OneMinusReflectivityMetallic(float1 metallic)
{
    // We'll need oneMinusReflectivity, so
    //   1-reflectivity = 1-lerp(dielectricSpec, 1, metallic) = lerp(1-dielectricSpec, 0, metallic)
    // store (1-dielectricSpec) in kDielectricSpec.a, then
    //   1-reflectivity = lerp(alpha, 0, metallic) = alpha + metallic*(0 - alpha) =
    //                  = alpha - metallic * alpha
    half oneMinusDielectricSpec = kDielectricSpec.a;
    return oneMinusDielectricSpec - metallic * oneMinusDielectricSpec;
}

float PerceptualSmoothnessToPerceptualRoughness(float perceptualSmoothness)
{
   return (1.0 - perceptualSmoothness);
}

float PerceptualRoughnessToRoughness(float perceptualRoughness)
{
     return perceptualRoughness * perceptualRoughness;
}

BRDFData InitializeBRDFData(float3 albedo, float metallic, float smoothness)
{
    BRDFData brdfData;

    float oneMinusReflectivity = OneMinusReflectivityMetallic(metallic);
    float reflectivity = 1.0 - oneMinusReflectivity;
    brdfData.diffuse = albedo * oneMinusReflectivity;
    brdfData.specular = lerp(kDielectricSpec.rgb, albedo, metallic);

    brdfData.perceptualRoughness = PerceptualSmoothnessToPerceptualRoughness(smoothness);
    brdfData.roughness = PerceptualRoughnessToRoughness(brdfData.perceptualRoughness);
    brdfData.roughness2 = brdfData.roughness * brdfData.roughness;
    brdfData.grazingTerm = saturate(smoothness + reflectivity);
    brdfData.normalizationTerm = brdfData.roughness * 4.0 + 2.0;
    brdfData.roughness2MinusOne = brdfData.roughness2 - 1.0;

    return brdfData;
}
