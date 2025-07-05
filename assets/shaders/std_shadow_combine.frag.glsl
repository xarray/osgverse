#pragma import_defines(VERSE_SHADOW_EYESPACE, VERSE_SHADOW_BAND_PCF, VERSE_SHADOW_POSSION_PCF)
#pragma import_defines(VERSE_SHADOW_VSM, VERSE_SHADOW_ESM, VERSE_SHADOW_EVSM)
#include "shadowing.module.glsl"

uniform sampler2D ColorBuffer, SsaoBlurredBuffer, NormalBuffer, DepthBuffer;
uniform sampler2D ShadowMap0, ShadowMap1, ShadowMap2, ShadowMap3;
uniform sampler2D RandomTexture;
uniform mat4 ShadowSpaceMatrices[VERSE_MAX_SHADOWS];
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
uniform vec2 InvShadowMapSize;
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

void main()
{
    vec2 uv0 = texCoord0.xy;
    vec4 colorData = VERSE_TEX2D(ColorBuffer, uv0);
    vec4 normalAlpha = VERSE_TEX2D(NormalBuffer, uv0);
    float depthValue = VERSE_TEX2D(DepthBuffer, uv0).r * 2.0 - 1.0;
    float ao = VERSE_TEX2D(SsaoBlurredBuffer, uv0).r;
    
    // Rebuild world vertex attributes
    vec4 vecInProj = vec4(uv0.x * 2.0 - 1.0, uv0.y * 2.0 - 1.0, depthValue, 1.0);
    vec4 eyeVertex = GBufferMatrices[3] * vecInProj;
    vec3 eyeNormal = normalAlpha.rgb;
    
    // Compute shadow and combine with color
    float shadow = 1.0;
    for (int i = 0; i < VERSE_MAX_SHADOWS; ++i)
    {
        vec4 lightProjVec = ShadowSpaceMatrices[i] * eyeVertex;
        vec2 lightProjUV = (lightProjVec.xy / lightProjVec.w) * 0.5 + vec2(0.5);
        if (any(lessThan(lightProjUV, vec2(0.0))) || any(greaterThan(lightProjUV, vec2(1.0)))) continue;
        
        float shadowValue = 1.0, depth = lightProjVec.z / lightProjVec.w;  // real depth in light space
        if (i == 0) shadowValue = GET_SHADOW(ShadowMap0, lightProjUV.xy, depth).z;
        else if (i == 1) shadowValue = GET_SHADOW(ShadowMap1, lightProjUV.xy, depth).z;
        else if (i == 2) shadowValue = GET_SHADOW(ShadowMap2, lightProjUV.xy, depth).z;
        else if (i == 3) shadowValue = GET_SHADOW(ShadowMap3, lightProjUV.xy, depth).z;
        shadow *= shadowValue;
    }
    
    colorData.rgb *= shadow * ao;
    fragData = colorData;
    VERSE_FS_FINAL(fragData);
}
