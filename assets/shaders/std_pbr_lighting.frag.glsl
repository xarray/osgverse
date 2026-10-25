#include "basic_lighting.module.glsl"

uniform sampler2D BrdfLutBuffer, PrefilterBuffer, IrradianceBuffer;
uniform sampler2D NormalBuffer, DepthBuffer, DiffuseMetallicBuffer, SpecularRoughnessBuffer;
uniform sampler2D SsaoBlurredBuffer;  // screen-space AO combined with material AO
#ifdef VERSE_LIGHT_BUFFER
// Texture buffer object (TBO) copy of the light parameter table: element (id, row) is stored at
// the flat index 'id + 1024 * row' (the same data the Texture2D fallback keeps in its 4 rows)
uniform samplerBuffer LightParameterBuffer;
#else
uniform sampler2D LightParameterMap;  // (r0: col+type, r1: pos+range, r2: dir+spotCutoff)
#endif
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
uniform vec2 InvScreenResolution, LightNumber;  // (num, max_num)
#ifdef VERSE_LIGHT_CLUSTER
uniform samplerBuffer LightIndexBuffer;    // light indices of each cluster, in a flat list
uniform samplerBuffer LightClusterBuffer;  // (offset, count) of the index list of each cluster
uniform vec3 LightClusterGrid;             // grid size: (tilesX, tilesY, slices)
uniform vec2 LightClusterTile;             // size of one screen tile, in pixels
uniform vec2 LightClusterDepth;            // (near, far) range the slices are distributed over
uniform int LightClusterEnabled;           // 1: loop over the lights of the current cluster only
#endif
VERSE_FS_IN vec4 texCoord0;

#if defined(VERSE_GLES3) || defined(VERSE_FS_MRT)
layout(location = 0) VERSE_FS_OUT vec4 fragData0;
layout(location = 1) VERSE_FS_OUT vec4 fragData1;
#endif

const vec2 invAtan = vec2(0.1591, 0.3183);
const int maxLights = 1024;
#ifdef VERSE_LIGHT_CLUSTER
const int maxClusterLights = 256;  // must match LightCluster::MaxLightsPerCluster
#endif

// Bound of the indirect contribution: the IBL textures are baked offline (see
// std_environment_prefiltering.frag.glsl, which documents what an unbounded environment radiance
// does to the whole chain) and a value which is not a number can still reach this shader from a
// broken texture or a degenerate tangent frame. The ambient term is the last place where it can
// be stopped before it reaches the tone mapping, where a NaN would blacken the pixel and be
// spread over a large patch by the bloom
const float MAX_AMBIENT_RADIANCE = 1000.0;

/// PBR functions
vec2 sphericalUV(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan; uv += 0.5; return uv;
}

// Equirectangular sky maps (and therefore the IBL data computed from them: the irradiance
// convolution and the prefiltered environment, both for the offline baked .ibl.rseq files and
// for the runtime generated buffers) are stored with their poles along the X axis, and the
// environment is always sampled after a -90 degree rotation about X, which is also what the
// sky box does when drawing the sky (see skybox.frag.glsl): rotate(-PI/2, X) maps (x, y, z)
// to (x, z, -y). The directions used to look up those IBL buffers have to be rotated in the
// same way, otherwise the ambient contribution would come from a part of the sky 90 degrees
// away from the one being displayed
vec3 skyMapDirection(vec3 v) { return vec3(v.x, v.z, -v.y); }

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

int getLightAttributes(in int id, out vec3 color, out vec3 pos, out vec3 dir,
                       out float range, out float spotCutoff)
{
#ifdef VERSE_LIGHT_BUFFER
    vec4 attr0 = texelFetch(LightParameterBuffer, id + 1024 * 0); // color, type
    vec4 attr1 = texelFetch(LightParameterBuffer, id + 1024 * 1); // pos, att
    vec4 attr2 = texelFetch(LightParameterBuffer, id + 1024 * 2); // dir, spot
#else
    const vec2 halfP = vec2(0.5 / 1024.0, 0.5 / 4.0), step = vec2(1.0 / 1024.0, 1.0 / 4.0);
    float fid = float(id);
    vec4 attr0 = VERSE_TEX2D(LightParameterMap, halfP + vec2(fid * step.x, 0.0 * step.y)); // color, type
    vec4 attr1 = VERSE_TEX2D(LightParameterMap, halfP + vec2(fid * step.x, 1.0 * step.y)); // pos, att
    vec4 attr2 = VERSE_TEX2D(LightParameterMap, halfP + vec2(fid * step.x, 2.0 * step.y)); // dir, spot
#endif
    color = attr0.xyz; pos = attr1.xyz; dir = attr2.xyz; range = attr1.w;
    spotCutoff = attr2.w; return int(attr0.w);
}

// Contribution of one light, dispatched on its type (see LightDrawable::Type)
vec3 getLightContribution(int type, vec3 viewDir, vec3 eyePosition, vec3 eyeNormal,
                          vec3 lightColor, vec3 lightPos, vec3 lightDir,
                          float lightRange, float lightSpot,
                          vec3 albedo, float metallic, float roughness)
{
    if (type == 1)
    {
        return get_directional_light_contribution(
                viewDir, eyePosition, lightPos, lightDir, lightColor, albedo, metallic, roughness,
                eyeNormal, lightRange);
    }
    else if (type == 2)
    {
        return get_point_light_contribution(
                viewDir, eyePosition, lightPos, lightColor, albedo, metallic, roughness,
                eyeNormal, lightRange);
    }
    else if (type == 3)
    {
        return get_spot_light_contribution(
                viewDir, eyePosition, lightPos, lightDir, lightColor, albedo, metallic, roughness,
                eyeNormal, lightRange, lightSpot);
    }
    return vec3(0.0, 0.0, 0.0);
}

#ifdef VERSE_LIGHT_CLUSTER
// Linear index of the cluster a fragment belongs to (see LightCluster::build(), which uses the
// same grid, the same exponential slices and the same near/far range on the CPU side)
int getClusterIndex(vec3 eyePosition)
{
    ivec2 gridXY = ivec2(LightClusterGrid.xy);
    ivec2 tile = ivec2(gl_FragCoord.xy / LightClusterTile);
    tile = clamp(tile, ivec2(0), gridXY - 1);
    float viewZ = max(-eyePosition.z, LightClusterDepth.x);
    float f = log(viewZ / LightClusterDepth.x) / log(LightClusterDepth.y / LightClusterDepth.x);
    int slices = int(LightClusterGrid.z);
    int slice = clamp(int(f * float(slices)), 0, slices - 1);
    return (slice * gridXY.y + tile.y) * gridXY.x + tile.x;
}
#endif

void main()
{
    vec2 uv0 = texCoord0.xy;
    float depthValue = VERSE_TEX2D(DepthBuffer, uv0).r * 2.0 - 1.0;

    // Background pixels (nothing was rendered into the G-Buffer, so the depth keeps its maximum
    // value) must not be shaded at all: rebuilding their eye-space position divides by a zero or
    // infinite w, which turns the view direction, and with it the whole light contribution, into
    // NaN. As the HDR buffers of this pipeline are floating point, such a NaN is no longer hidden
    // by an 8-bit conversion: the bloom blurs it around (black edges along the silhouettes) and
    // the auto-exposure reads it as the frame luminance, which blacks out every pixel of the
    // deferred image. The background is painted later by the deferred sky or by the sky box
    // camera, so 0 is written here
    if (depthValue >= 1.0)
    {
#if defined(VERSE_GLES3) || defined(VERSE_FS_MRT)
        fragData0/*ColorBuffer*/ = vec4(0.0, 0.0, 0.0, 1.0);
        fragData1/*IblAmbientBuffer*/ = vec4(0.0, 0.0, 0.0, 1.0);
#else
        gl_FragData[0]/*ColorBuffer*/ = vec4(0.0, 0.0, 0.0, 1.0);
        gl_FragData[1]/*IblAmbientBuffer*/ = vec4(0.0, 0.0, 0.0, 1.0);
#endif
        return;
    }

    vec4 diffuseMetallic = VERSE_TEX2D(DiffuseMetallicBuffer, uv0);
    vec4 specularRoughness = VERSE_TEX2D(SpecularRoughnessBuffer, uv0);
    vec4 normalAlpha = VERSE_TEX2D(NormalBuffer, uv0);

    // Rebuild world vertex attributes. The result of the projection-to-view matrix is homogeneous
    // (its w term is not 1), so the division is mandatory: without it the eye-space position fed
    // to the point/spot light functions would be scaled towards the origin, giving wrong light
    // directions and attenuation distances
    vec4 vecInProj = vec4(uv0.x * 2.0 - 1.0, uv0.y * 2.0 - 1.0, depthValue, 1.0);
    vec4 eyeVertex = GBufferMatrices[3] * vecInProj;
    vec3 eyePosition = eyeVertex.xyz / eyeVertex.w;
    vec3 eyeNormal = normalAlpha.rgb;

    // Components common to all light types
    vec3 viewDir = -normalize(eyePosition);
    vec3 R = reflect(-viewDir, eyeNormal);
    vec3 albedo = pow(diffuseMetallic.rgb, vec3(2.2)), specular = specularRoughness.rgb;
    float metallic = diffuseMetallic.a, roughness = specularRoughness.a, ao = normalAlpha.a;
    float nDotV = max(dot(eyeNormal, viewDir), 0.0);

    // Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 of 0.04;
    // if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = mix(vec3(0.04), albedo, metallic), radianceOut = vec3(0.0);

    // Compute lights: either the ones assigned to the cluster of this pixel (clustered light
    // culling, computed on the CPU by LightCluster), or the whole table when it is disabled
    vec3 lightColor, lightPos, lightDir; float lightRange = 0.0, lightSpot = 0.0;
#ifdef VERSE_LIGHT_CLUSTER
    if (LightClusterEnabled == 1)
    {
        vec2 clusterRange = texelFetch(LightClusterBuffer, getClusterIndex(eyePosition)).rg;
        int start = int(clusterRange.x), count = int(clusterRange.y);
        for (int i = 0; i < maxClusterLights; ++i)
        {
            if (i >= count) break;  // to avoid 'WebGL: Loop index cannot be compared with non-constant expression'
            int id = int(texelFetch(LightIndexBuffer, start + i).r);
            int type = getLightAttributes(id, lightColor, lightPos, lightDir, lightRange, lightSpot);
            radianceOut += getLightContribution(
                    type, viewDir, eyePosition, eyeNormal, lightColor, lightPos, lightDir,
                    lightRange, lightSpot, albedo, metallic, roughness);
        }
    }
    else
#endif
    {
        int numLights = int(min(LightNumber.x, LightNumber.y));
        for (int i = 0; i < maxLights; ++i)
        {
            if (numLights <= i) break;  // to avoid 'WebGL: Loop index cannot be compared with non-constant expression'
            int type = getLightAttributes(i, lightColor, lightPos, lightDir, lightRange, lightSpot);
            radianceOut += getLightContribution(
                    type, viewDir, eyePosition, eyeNormal, lightColor, lightPos, lightDir,
                    lightRange, lightSpot, albedo, metallic, roughness);
        }
    }

    // Treat ambient light as IBL (only shaded pixels reach this point, see the check above)
    vec3 kS = fresnelSchlickRoughness(nDotV, F0, roughness);
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    vec3 irradiance = VERSE_TEX2D(IrradianceBuffer, sphericalUV(skyMapDirection(eyeNormal))).rgb;
    vec3 diffuse = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(
        PrefilterBuffer, sphericalUV(skyMapDirection(R)), roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = VERSE_TEX2D(BrdfLutBuffer, vec2(nDotV, roughness)).rg;
    vec3 envSpecular = prefilteredColor * (kS * envBRDF.x + envBRDF.y);

    // Occlusion of the ambient/indirect lighting: material AO (stored in NormalBuffer.a)
    // multiplied by the screen-space AO. Note it is applied to indirect light only, which
    // is the physically correct place; direct light is handled by the shadowing stage.
    float occlusion = ao * VERSE_TEX2D(SsaoBlurredBuffer, uv0).r;
    vec3 ambient = kD * diffuse * occlusion + envSpecular;

    // Dropping the NaN of a broken IBL texture here (instead of letting it reach the tone mapping)
    // is what keeps such a pixel from becoming a black patch: the luminance of the ambient is used
    // as the test, as a NaN in any of its components propagates to it
    float ambientLum = dot(ambient, vec3(0.2126, 0.7152, 0.0722));
    if (ambientLum != ambientLum) ambient = vec3(0.0);
    else ambient = min(ambient, vec3(MAX_AMBIENT_RADIANCE));

#if defined(VERSE_GLES3) || defined(VERSE_FS_MRT)
    fragData0/*ColorBuffer*/ = vec4(radianceOut, 1.0);
    fragData1/*IblAmbientBuffer*/ = vec4(ambient, 1.0);
#else
    gl_FragData[0]/*ColorBuffer*/ = vec4(radianceOut, 1.0);
    gl_FragData[1]/*IblAmbientBuffer*/ = vec4(ambient, 1.0);
#endif
}
