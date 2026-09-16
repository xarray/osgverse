#pragma import_defines(VERSE_SHADOW_EYESPACE, VERSE_SHADOW_BAND_PCF, VERSE_SHADOW_POSSION_PCF)
#pragma import_defines(VERSE_SHADOW_VSM, VERSE_SHADOW_ESM, VERSE_SHADOW_EVSM)
#include "shadowing.module.glsl"

uniform sampler2D ColorBuffer, NormalBuffer, DepthBuffer;
uniform sampler2D ShadowMap0, ShadowMap1, ShadowMap2, ShadowMap3;
uniform sampler2D RandomTexture;
uniform mat4 ShadowSpaceMatrices[VERSE_MAX_SHADOWS];
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
uniform vec2 InvShadowMapSize;
uniform vec2 CascadeInfo;  // (number of cascades, ratio of the blending band)
uniform vec4 CascadeFarDepths;  // far view distance of each cascade
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

#define GET_SHADOW(map, uv, z) getShadowValue(map, uv, z)
#ifdef VERSE_SHADOW_POSSION_PCF
#   undef GET_SHADOW
#   define GET_SHADOW(map, uv, z) getShadowValue_PossionPCF(map, RandomTexture, uv, z, InvShadowMapSize)
#endif
#ifdef VERSE_SHADOW_BAND_PCF
#   undef GET_SHADOW
#   define GET_SHADOW(map, uv, z) getShadowValue_BandPCF(map, uv, z, InvShadowMapSize)
#endif
#ifdef VERSE_SHADOW_VSM
#   undef GET_SHADOW
#   define GET_SHADOW(map, uv, z) getShadowValue_VSM(map, uv, z, 0.0008)
#endif
#ifdef VERSE_SHADOW_ESM
#   undef GET_SHADOW
#   define GET_SHADOW(map, uv, z) getShadowValue_ESM(map, uv, z, 0.33, 15.0)
#endif
#ifdef VERSE_SHADOW_EVSM
#   undef GET_SHADOW
#   define GET_SHADOW(map, uv, z) getShadowValue_EVSM(map, uv, z, 0.33, 15.0, 0.0008)
#endif

float getCascadeShadowValue(in sampler2D shadowMap, in vec4 lightProjVec)
{
    vec2 lightProjUV = (lightProjVec.xy / lightProjVec.w) * 0.5 + vec2(0.5);
    if (any(lessThan(lightProjUV, vec2(0.0))) || any(greaterThan(lightProjUV, vec2(1.0))))
        return 1.0;  // outside of this cascade: treated as unshadowed

    float depth = lightProjVec.z / lightProjVec.w;  // real depth in light space
    return GET_SHADOW(shadowMap, lightProjUV.xy, depth).z;
}

void main()
{
    vec2 uv0 = texCoord0.xy;
    vec4 colorData = VERSE_TEX2D(ColorBuffer, uv0);
    vec4 normalAlpha = VERSE_TEX2D(NormalBuffer, uv0);
    float depthValue = VERSE_TEX2D(DepthBuffer, uv0).r * 2.0 - 1.0;
    
    // Rebuild world vertex attributes
    vec4 vecInProj = vec4(uv0.x * 2.0 - 1.0, uv0.y * 2.0 - 1.0, depthValue, 1.0);
    vec4 eyeVertex = GBufferMatrices[3] * vecInProj;
    vec3 eyeNormal = normalAlpha.rgb;
    
    // Select the cascade covering this pixel by its view distance. Shadow maps of different
    // cascades have different resolutions, so multiplying all overlapping cascades (as it was
    // done before) would always degrade the result to the coarsest one
    float viewDepth = -(eyeVertex.z / eyeVertex.w);
    int numCascades = int(CascadeInfo.x), cascadeID = -1;
    float cascadeNear = 0.0, cascadeFar = 0.0;
    for (int i = 0; i < VERSE_MAX_SHADOWS; ++i)
    {
        if (i >= numCascades) break;
        if (viewDepth < CascadeFarDepths[i])
        { cascadeID = i; cascadeFar = CascadeFarDepths[i]; break; }
        cascadeNear = CascadeFarDepths[i];
    }

    // Compute shadow and combine with color
    float shadow = 1.0;
    if (cascadeID >= 0)
    {
        // Blend to the next cascade (or fade out for the last one) near the far edge of the
        // selected cascade, to avoid a visible transition between cascades
        float fadeStart = mix(cascadeNear, cascadeFar, 1.0 - CascadeInfo.y);
        float blend = clamp((viewDepth - fadeStart) / max(cascadeFar - fadeStart, 0.0001), 0.0, 1.0);
        bool needNext = (blend > 0.0) && (cascadeID < numCascades - 1);
        float shadowThis = 1.0, shadowNext = 1.0;

        if (cascadeID == 0)
        {
            shadowThis = getCascadeShadowValue(ShadowMap0, ShadowSpaceMatrices[0] * eyeVertex);
            if (needNext) shadowNext = getCascadeShadowValue(ShadowMap1, ShadowSpaceMatrices[1] * eyeVertex);
        }
        else if (cascadeID == 1)
        {
            shadowThis = getCascadeShadowValue(ShadowMap1, ShadowSpaceMatrices[1] * eyeVertex);
            if (needNext) shadowNext = getCascadeShadowValue(ShadowMap2, ShadowSpaceMatrices[2] * eyeVertex);
        }
        else if (cascadeID == 2)
        {
            shadowThis = getCascadeShadowValue(ShadowMap2, ShadowSpaceMatrices[2] * eyeVertex);
            if (needNext) shadowNext = getCascadeShadowValue(ShadowMap3, ShadowSpaceMatrices[3] * eyeVertex);
        }
        else
            shadowThis = getCascadeShadowValue(ShadowMap3, ShadowSpaceMatrices[3] * eyeVertex);
        shadow = mix(shadowThis, shadowNext, blend);
    }

    // Shadows only affect the direct lighting result; ambient occlusion of the indirect
    // (IBL) term has already been applied by the lighting stage
    colorData.rgb *= shadow;
    fragData = colorData;
    VERSE_FS_FINAL(fragData);
}
