//
// This fragment shader defines a reference implementation for Physically Based Shading of
// a microfacet surface material defined by a glTF model.
//
// References:
// [1] Real Shading in Unreal Engine 4
//     http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// [2] Physically Based Shading at Disney
//     http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
// [3] README.md - Environment Maps
//     https://github.com/KhronosGroup/glTF-WebGL-PBR/#environment-maps
// [4] "An Inexpensive BRDF Model for Physically based Rendering" by Christophe Schlick
//     https://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf

// KHR_lights_punctual extension.
// see https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual

// TODO: maybe move me into definitions
#ifndef HAVE_PBRLIGHT_STRUCT
#define HAVE_PBRLIGHT_STRUCT
struct PBRLight {
    int enabled;

    vec3 direction;
    float range;

    vec3 color;
    float intensity;

    vec3 position;
    float innerConeCos;

    float outerConeCos;
    int type;
};
#endif

const int LightType_Directional = 0;
const int LightType_Point = 1;
const int LightType_Spot = 2;

#ifdef USE_PUNCTUAL
uniform PBRLight u_PBRLights[LIGHT_COUNT];
#endif

struct PBRMaterial {
	float metallicFactor;
	float roughnessFactor;
	vec4 baseColorFactor;
	float exposure;
	float alphaCutoff;
	int alphaCutoffEnabled;
	#ifdef MATERIAL_SPECULARGLOSSINESS
		vec3 specularFactor;
		vec4 diffuseFactor;
		float glossinessFactor;
	#endif
	int normalSamplerAvailable;
	int baseColorSamplerAvailable;
	int metallicRoughnessSamplerAvailable;
	vec3 normal;
	#ifdef MATERIAL_SPECULARGLOSSINESS
		vec4 specularGlossinessColor
	#endif
	#ifdef HAS_DIFFUSE_MAP
		vec4 diffuseColor;
	#endif
	vec4 vertexColor;
	vec4 metallicRoughnessColor;
	vec4 baseColor;
	#ifdef DEBUG_NORMAL
		vec3 normalColor;
	#endif
};

uniform vec3 u_Camera;

uniform int u_MipCount;

struct MaterialInfo {
    float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
    vec3 reflectance0;            // full reflectance color (normal incidence angle)

    float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
    vec3 diffuseColor;            // color contribution from diffuse lighting

    vec3 reflectance90;           // reflectance color at grazing angle
    vec3 specularColor;           // color contribution from specular lighting
};

// Calculation of the lighting contribution from an optional Image Based PBRLight source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
#ifdef USE_IBL
uniform samplerCube u_DiffuseEnvSampler;
uniform samplerCube u_SpecularEnvSampler;
uniform sampler2D u_brdfLUT;

vec3 getIBLContribution(MaterialInfo materialInfo, vec3 n, vec3 v)
{
    float NdotV = clamp(dot(n, v), 0.0, 1.0);

    vec3 reflection = normalize(reflect(-v, n));

    vec2 brdfSamplePoint = clamp(vec2(NdotV, materialInfo.perceptualRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
    // retrieve a scale and bias to F0. See [1], Figure 3
    vec2 brdf = texture(u_brdfLUT, brdfSamplePoint).rg;

    vec4 diffuseSample = texture(u_DiffuseEnvSampler, n);

#ifdef USE_TEX_LOD
    float lod = clamp(materialInfo.perceptualRoughness * float(u_MipCount), 0.0, float(u_MipCount));
    vec4 specularSample = textureLodEXT(u_SpecularEnvSampler, reflection, lod);
#else
    vec4 specularSample = texture(u_SpecularEnvSampler, reflection);
#endif

#ifdef USE_HDR
    // Already linear.
    vec3 diffuseLight = diffuseSample.rgb;
    vec3 specularLight = specularSample.rgb;
#else
    vec3 diffuseLight = SRGBtoLINEAR(diffuseSample).rgb;
    vec3 specularLight = SRGBtoLINEAR(specularSample).rgb;
#endif

    vec3 diffuse = diffuseLight * materialInfo.diffuseColor;
    vec3 specular = specularLight * (materialInfo.specularColor * brdf.x + brdf.y);

    return diffuse + specular * 0.00000001;
}
#endif

// Lambert lighting
// see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
vec3 diffuse(MaterialInfo materialInfo)
{
    return materialInfo.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    return materialInfo.reflectance0 + (materialInfo.reflectance90 - materialInfo.reflectance0) * pow(clamp(1.0 - angularInfo.VdotH, 0.0, 1.0), 5.0);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float visibilityOcclusion(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    float NdotL = angularInfo.NdotL;
    float NdotV = angularInfo.NdotV;
    float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(MaterialInfo materialInfo, AngularInfo angularInfo)
{
    float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;
    float f = (angularInfo.NdotH * alphaRoughnessSq - angularInfo.NdotH) * angularInfo.NdotH + 1.0;
    return alphaRoughnessSq / (M_PI * f * f);
}

vec3 getPointShade(vec3 pointToLight, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
    AngularInfo angularInfo = getAngularInfo(pointToLight, normal, view);

    if (angularInfo.NdotL > 0.0 || angularInfo.NdotV > 0.0)
    {
        // Calculate the shading terms for the microfacet specular shading model
        vec3 F = specularReflection(materialInfo, angularInfo);
        float Vis = visibilityOcclusion(materialInfo, angularInfo);
        float D = microfacetDistribution(materialInfo, angularInfo);

        // Calculation of analytical lighting contribution
        vec3 diffuseContrib = (1.0 - F) * diffuse(materialInfo);
        vec3 specContrib = F * Vis * D;

        // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
        return angularInfo.NdotL * (diffuseContrib + specContrib);
    }

    return vec3(0.0, 0.0, 0.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
float getRangeAttenuation(float range, float distance)
{
    if (range <= 0.0)
    {
        // negative range means unlimited
        return 1.0;
    }
    return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
float getSpotAttenuation(vec3 pointToLight, vec3 spotDirection, float outerConeCos, float innerConeCos)
{
    float actualCos = dot(normalize(spotDirection), normalize(-pointToLight));
    if (actualCos > outerConeCos)
    {
        if (actualCos < innerConeCos)
        {
            return smoothstep(outerConeCos, innerConeCos, actualCos);
        }
        return 1.0;
    }
    return 0.0;
}

vec3 applyDirectionalLight(PBRLight light, MaterialInfo materialInfo, vec3 normal, vec3 view)
{
    vec3 pointToLight = -light.direction;
    vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return light.intensity * light.color * shade;
}

vec3 applyPointLight(PBRLight light, MaterialInfo materialInfo, vec3 position, vec3 normal, vec3 view)
{
    vec3 pointToLight = light.position - position;
    float distance = length(pointToLight);
    float attenuation = getRangeAttenuation(light.range, distance);
    vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return attenuation * light.intensity * light.color * shade;
}

vec3 applySpotLight(PBRLight light, MaterialInfo materialInfo, vec3 position, vec3 normal, vec3 view)
{
    vec3 pointToLight = light.position - position;
    float distance = length(pointToLight);
    float rangeAttenuation = getRangeAttenuation(light.range, distance);
    float spotAttenuation = getSpotAttenuation(pointToLight, light.direction, light.outerConeCos, light.innerConeCos);
    vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
    return rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;
}

vec4 computePBRLighting(in vec3 position, in PBRMaterial pbrMaterial)
{
    // resulting fragment color
    vec4 outColor = vec4(0.0, 0.0, 0.0, 1.0);

    // Metallic and Roughness material properties are packed together
    // In glTF, these factors can be specified by fixed scalar values
    // or from a metallic-roughness map
    float perceptualRoughness = 0.0;
    float metallic = 0.0;
    vec4 baseColor = vec4(0.0, 0.0, 0.0, 1.0);
    vec3 diffuseColor = vec3(0.0);
    vec3 specularColor= vec3(0.0);
    vec3 f0 = vec3(0.04);

#ifdef MATERIAL_SPECULARGLOSSINESS

#ifdef HAS_SPECULAR_GLOSSINESS_MAP
    vec4 sgSample = SRGBtoLINEAR(pbrMaterial.specularGlossinessColor);
    perceptualRoughness = (1.0 - sgSample.a * pbrMaterial.glossinessFactor); // glossiness to roughness
    f0 = sgSample.rgb * pbrMaterial.specularFactor; // specular
#else
    f0 = pbrMaterial.specularFactor;
    perceptualRoughness = 1.0 - pbrMaterial.glossinessFactor;
#endif // ! HAS_SPECULAR_GLOSSINESS_MAP

#ifdef HAS_DIFFUSE_MAP
    baseColor = SRGBtoLINEAR(pbrMaterial.diffuseColor) * pbrMaterial.diffuseFactor;
#else
    baseColor = pbrMaterial.diffuseFactor;
#endif // !HAS_DIFFUSE_MAP

    baseColor *= pbrMaterial.vertexColor;

    // f0 = specular
    specularColor = f0;
    float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);
    diffuseColor = baseColor.rgb * oneMinusSpecularStrength;

#ifdef DEBUG_METALLIC
    // do conversion between metallic M-R and S-G metallic
    metallic = solveMetallic(baseColor.rgb, specularColor, oneMinusSpecularStrength);
#endif // ! DEBUG_METALLIC

#endif // ! MATERIAL_SPECULARGLOSSINESS

#ifdef MATERIAL_METALLICROUGHNESS
    // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
    // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
    vec4 mrSample = pbrMaterial.metallicRoughnessColor;
    perceptualRoughness = mrSample.g * pbrMaterial.roughnessFactor;
    metallic = mrSample.b * pbrMaterial.metallicFactor;

    // The albedo may be defined from a base texture or a flat color
    baseColor = SRGBtoLINEAR(pbrMaterial.baseColor) * pbrMaterial.baseColorFactor;

    baseColor *= pbrMaterial.vertexColor;

    diffuseColor = baseColor.rgb * (vec3(1.0) - f0) * (1.0 - metallic);

    specularColor = mix(f0, baseColor.rgb, metallic);

#endif // ! MATERIAL_METALLICROUGHNESS

    if (pbrMaterial.alphaCutoffEnabled == 1) {
		if (baseColor.a < pbrMaterial.alphaCutoff) discard;
		baseColor.a = 1.0;
    }

#ifdef MATERIAL_UNLIT
    outColor = vec4(LINEARtoSRGB(baseColor.rgb), baseColor.a);
    return;
#endif

    perceptualRoughness = clamp(perceptualRoughness, 0.0, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    float alphaRoughness = perceptualRoughness * perceptualRoughness;

    // Compute reflectance.
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

    vec3 specularEnvironmentR0 = specularColor.rgb;
    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
    vec3 specularEnvironmentR90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));

    MaterialInfo materialInfo = MaterialInfo(
        perceptualRoughness,
        specularEnvironmentR0,
        alphaRoughness,
        diffuseColor,
        specularEnvironmentR90,
        specularColor
    );

    // LIGHTING

    vec3 color = vec3(0.0, 0.0, 0.0);
    vec3 normal = pbrMaterial.normal;
    vec3 view = normalize(u_Camera - position);

#ifdef USE_PUNCTUAL
    for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        PBRLight light = u_PBRLights[i];

        if (light.enabled == 0) continue;

        if (light.type == LightType_Directional)
        {
            color += applyDirectionalLight(light, materialInfo, normal, view);
        }
        else if (light.type == LightType_Point)
        {
            color += applyPointLight(light, materialInfo, position, normal, view);
        }
        else if (light.type == LightType_Spot)
        {
            color += applySpotLight(light, materialInfo, position, normal, view);
        }
    }
#endif

    // Calculate lighting contribution from image based lighting source (IBL)
#ifdef USE_IBL
    color += getIBLContribution(materialInfo, normal, view);
#endif

    float ao = 1.0;
    // Apply optional PBR terms for additional (optional) shading
#ifdef HAS_OCCLUSION_MAP
    ao = pbrMaterial.occlusionColor.r;
    color = mix(color, color * ao, pbrMaterial.occlusionStrength);
#endif

    vec3 emissive = vec3(0);
#ifdef HAS_EMISSIVE_MAP
    emissive = SRGBtoLINEAR(pbrMaterial.emissiveColor).rgb * pbrMaterial.emissiveFactor;
    color += emissive;
#endif

#ifndef DEBUG_OUTPUT // no debug

   // regular shading
    outColor = vec4(toneMap(color, pbrMaterial.exposure), baseColor.a);

#else // debug output

    #ifdef DEBUG_METALLIC
        outColor.rgb = vec3(metallic);
    #endif

    #ifdef DEBUG_ROUGHNESS
        outColor.rgb = vec3(perceptualRoughness);
    #endif

    #ifdef DEBUG_NORMAL
        if (pbrMaterial.normalSamplerAvailable == 1) {
            outColor.rgb = pbrMaterial.normalColor.rgb;
        } else {
            outColor.rgb = vec3(0.5, 0.5, 1.0);
        }
    #endif

    #ifdef DEBUG_BASECOLOR
        outColor.rgb = LINEARtoSRGB(baseColor.rgb);
    #endif

    #ifdef DEBUG_OCCLUSION
        outColor.rgb = vec3(ao);
    #endif

    #ifdef DEBUG_EMISSIVE
        outColor.rgb = LINEARtoSRGB(emissive);
    #endif

    #ifdef DEBUG_F0
        outColor.rgb = vec3(f0);
    #endif

    #ifdef DEBUG_ALPHA
        outColor.rgb = vec3(baseColor.a);
    #endif

    outColor.a = 1.0;
#endif // !DEBUG_OUTPUT

    //
    return outColor;
}
