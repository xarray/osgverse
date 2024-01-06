#include "module_lighting.glsl"

uniform sampler2D BrdfLutBuffer, PrefilterBuffer, IrradianceBuffer;
uniform sampler2D NormalBuffer, DepthBuffer, DiffuseMetallicBuffer;
uniform sampler2D SpecularRoughnessBuffer, EmissionOcclusionBuffer;
uniform sampler2D LightParameterMap;  // (r0: col+type, r1: pos+att1, r2: dir+att0, r3: spotProp)
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
uniform vec2 InvScreenResolution, LightNumber;  // (num, max_num)
VERSE_FS_IN vec4 texCoord0;

#ifdef VERSE_GLES3
layout(location = 0) VERSE_FS_OUT vec4 fragData0;
layout(location = 1) VERSE_FS_OUT vec4 fragData1;
#endif

const vec2 invAtan = vec2(0.1591, 0.3183);
const int maxLights = 1024;

/// PBR functions
vec2 sphericalUV(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan; uv += 0.5; return uv;
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    float val = 1.0 - cosTheta;
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * (val*val*val*val*val); //Faster than pow
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    float val = 1.0 - cosTheta;
    return F0 + (1.0 - F0) * (val*val*val*val*val); //Faster than pow
}

int getLightAttributes(in float id, out vec3 color, out vec3 pos, out vec3 dir,
                       out float range, out float spotCutoff)
{
    const vec2 halfP = vec2(0.5 / 1024.0, 0.5 / 4.0), step = vec2(1.0 / 1024.0, 1.0 / 4.0);
    vec4 attr0 = VERSE_TEX2D(LightParameterMap, halfP + vec2(id * step.x, 0.0 * step.y)); // color, type
    vec4 attr1 = VERSE_TEX2D(LightParameterMap, halfP + vec2(id * step.x, 1.0 * step.y)); // pos, att
    vec4 attr2 = VERSE_TEX2D(LightParameterMap, halfP + vec2(id * step.x, 2.0 * step.y)); // dir, spot
    color = attr0.xyz; pos = attr1.xyz; dir = attr2.xyz; range = attr1.w;
    spotCutoff = attr2.w; return int(attr0.w);
}

void main()
{
    vec2 uv0 = texCoord0.xy;
    vec4 diffuseMetallic = VERSE_TEX2D(DiffuseMetallicBuffer, uv0);
    vec4 specularRoughness = VERSE_TEX2D(SpecularRoughnessBuffer, uv0);
    vec4 emissionOcclusion = VERSE_TEX2D(EmissionOcclusionBuffer, uv0);
    vec4 normalAlpha = VERSE_TEX2D(NormalBuffer, uv0);
    float depthValue = VERSE_TEX2D(DepthBuffer, uv0).r * 2.0 - 1.0;

    // Rebuild world vertex attributes
    vec4 vecInProj = vec4(uv0.x * 2.0 - 1.0, uv0.y * 2.0 - 1.0, depthValue, 1.0);
    vec4 eyeVertex = GBufferMatrices[3] * vecInProj;
    vec3 eyeNormal = normalAlpha.rgb;

    // Components common to all light types
    vec3 viewDir = -normalize(eyeVertex.xyz / eyeVertex.w);
    vec3 R = reflect(-viewDir, eyeNormal);
    vec3 albedo = pow(diffuseMetallic.rgb, vec3(2.2));
    vec3 specular = specularRoughness.rgb, emission = emissionOcclusion.rgb;
    float metallic = diffuseMetallic.a, roughness = specularRoughness.a, ao = emissionOcclusion.a;
    float nDotV = max(dot(eyeNormal, viewDir), 0.0);

    // Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 of 0.04;
    // if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = mix(vec3(0.04), albedo, metallic), radianceOut = vec3(0.0);

    // Compute direcional lights
    vec3 lightColor, lightPos, lightDir; float lightRange = 0.0, lightSpot = 0.0;
    int numLights = int(min(LightNumber.x, LightNumber.y));
    for (int i = 0; i < maxLights; ++i)
    {
        if (numLights <= i) break;  // to avoid 'WebGL: Loop index cannot be compared with non-constant expression'
        int type = getLightAttributes(float(i), lightColor, lightPos, lightDir, lightRange, lightSpot);
        if (type == 1)
        {
            //radianceOut += computeDirectionalLight(
            //      lightDir, lightColor, eyeNormal, viewDir, albedo, specular, roughness, metallic, F0);
            radianceOut += get_directional_light_contribution(
                    viewDir, eyeVertex.xyz, lightPos, lightDir, lightColor, albedo, metallic, roughness,
                    eyeNormal, lightRange);
        }
        else if (type == 2)
        {
            radianceOut += get_point_light_contribution(
                    viewDir, eyeVertex.xyz, lightPos, lightColor, albedo, metallic, roughness, eyeNormal, lightRange);
        }
        else if (type == 3)
        {
            radianceOut += get_spot_light_contribution(
                    viewDir, eyeVertex.xyz, lightPos, lightDir, lightColor, albedo, metallic, roughness,
                    eyeNormal, lightRange, lightSpot);
        }
    }

    // Treat ambient light as IBL
    vec3 ambient = vec3(0.025) * albedo;
    if (depthValue < 1.0)
    {
        vec3 kS = fresnelSchlickRoughness(nDotV, F0, roughness);
        vec3 kD = (1.0 - kS) * (1.0 - metallic);
        vec3 irradiance = VERSE_TEX2D(IrradianceBuffer, sphericalUV(eyeNormal)).rgb;
        vec3 diffuse = irradiance * albedo;

        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(PrefilterBuffer, sphericalUV(R), roughness * MAX_REFLECTION_LOD).rgb;
        vec2 envBRDF = VERSE_TEX2D(BrdfLutBuffer, vec2(nDotV, roughness)).rg;
        vec3 envSpecular = prefilteredColor * (kS * envBRDF.x + envBRDF.y);
        ambient = kD * diffuse + envSpecular;
    }

    ao = 1.0;  // FIXME: sponza seems to have a negative AO?
#ifdef VERSE_GLES3
    fragData0/*ColorBuffer*/ = vec4(radianceOut * pow(ao, 2.2), 1.0);
    fragData1/*IblAmbientBuffer*/ = vec4(ambient + emission, 1.0);
#else
    gl_FragData[0]/*ColorBuffer*/ = vec4(radianceOut * pow(ao, 2.2), 1.0);
    gl_FragData[1]/*IblAmbientBuffer*/ = vec4(ambient + emission, 1.0);
#endif
}
