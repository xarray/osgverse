#include "basic_lighting.module.glsl"

#define M_PI 3.1415926535897932384626433832795
uniform sampler2D DiffuseMap, NormalMap, SpecularMap, ShininessMap;
uniform sampler2D AmbientMap, EmissiveMap, ReflectionMap;
uniform sampler2D LightParameterMap;  // (r0: col+type, r1: pos+att1, r2: dir+att0, r3: spotProp)
uniform vec2 LightNumber;  // (num, max_num)
VERSE_FS_IN vec4 texCoord0, texCoord1, color, eyeVertex;
VERSE_FS_IN vec3 eyeNormal, eyeTangent, eyeBinormal;
VERSE_FS_OUT vec4 fragData;

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
    vec2 uv0 = texCoord0.xy, uv1 = texCoord1.xy;
    vec4 diffuse = VERSE_TEX2D(DiffuseMap, uv0) * color;
    vec4 normalValue = VERSE_TEX2D(NormalMap, uv0);
    vec4 emission = VERSE_TEX2D(EmissiveMap, uv1);
    vec3 specular = VERSE_TEX2D(SpecularMap, uv0).rgb;
    vec3 metalRough = VERSE_TEX2D(ShininessMap, uv0).rgb;

    // Compute eye-space normal
    vec3 eyeNormal2 = eyeNormal;
    if (normalValue.a > 0.1)
    {
        vec3 tsNormal = normalize(2.0 * normalValue.rgb - vec3(1.0));
        eyeNormal2 = normalize(mat3(eyeTangent, eyeBinormal, eyeNormal) * tsNormal);
    }

    // Components common to all light types
    vec3 viewDir = -normalize(eyeVertex.xyz / eyeVertex.w);
    vec3 R = reflect(-viewDir, eyeNormal2), albedo = diffuse.rgb;
    float metallic = metalRough.b, roughness = metalRough.g, ao = metalRough.r;
    float nDotV = max(dot(eyeNormal2, viewDir), 0.0);

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

    vec3 ambient = vec3(0.25) * albedo;
    radianceOut = mix(radianceOut, radianceOut * emission.rgb, emission.a);
    ao = 1.0;  // FIXME: sponza seems to have a negative AO?
    fragData = vec4(ambient + radianceOut * pow(ao, 2.2), diffuse.a);
    VERSE_FS_FINAL(fragData);
}
