#pragma import_defines(VERSE_SHADOW_EYESPACE, VERSE_SHADOW_BAND_PCF, VERSE_SHADOW_POSSION_PCF)
#pragma import_defines(VERSE_SHADOW_VSM, VERSE_SHADOW_ESM, VERSE_SHADOW_EVSM, VERSE_SHADOW_DEBUGCOLOR)
#include "shadowing.module.glsl"

uniform sampler2D ColorBuffer, SsaoBlurredBuffer, NormalBuffer, DepthBuffer;
uniform sampler2D ShadowMap0, ShadowMap1, ShadowMap2, ShadowMap3;
uniform sampler2D RandomTexture;
uniform mat4 ShadowSpaceMatrices[VERSE_MAX_SHADOWS];
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
uniform vec2 InvShadowMapSize;
VERSE_FS_IN vec4 texCoord0;

#ifdef VERSE_GLES3
layout(location = 0) VERSE_FS_OUT vec4 fragData;
layout(location = 1) VERSE_FS_OUT vec4 dbgDepth;
#endif

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
#ifdef VERSE_SHADOW_DEBUGCOLOR
    vec3 shadowColors[VERSE_MAX_SHADOWS], debugShadowColor = vec3(1, 1, 1);
    shadowColors[0] = vec3(1, 0, 0); shadowColors[1] = vec3(0, 1, 0);
    shadowColors[2] = vec3(0, 0, 1); shadowColors[3] = vec3(1, 1, 0);
#endif
    float shadow = 1.0; vec3 debugValue = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < VERSE_MAX_SHADOWS; ++i)
    {
        vec4 lightProjVec = ShadowSpaceMatrices[i] * eyeVertex;
        vec2 lightProjUV = (lightProjVec.xy / lightProjVec.w) * 0.5 + vec2(0.5);
        if (any(lessThan(lightProjUV, vec2(0.0))) || any(greaterThan(lightProjUV, vec2(1.0)))) continue;
        
        float depth = lightProjVec.z / lightProjVec.w;  // real depth in light space
        vec3 shadowValue = vec3(1.0);
        if (i == 0) shadowValue = GET_SHADOW(ShadowMap0, lightProjUV.xy, depth);
        else if (i == 1) shadowValue = GET_SHADOW(ShadowMap1, lightProjUV.xy, depth);
        else if (i == 2) shadowValue = GET_SHADOW(ShadowMap2, lightProjUV.xy, depth);
        else if (i == 3) shadowValue = GET_SHADOW(ShadowMap3, lightProjUV.xy, depth);

        if (length(debugValue) == 0.0) debugValue = vec3(
            (shadowValue.x - shadowValue.y) * 10.0, (abs(shadowValue.x - shadowValue.y) < 0.01) ? 1.0 : 0.0, 0.0);
        shadow *= shadowValue.z;
#ifdef VERSE_SHADOW_DEBUGCOLOR
        if (shadowValue.z < 0.5) debugShadowColor = shadowColors[i];
#endif
    }
    
#ifdef VERSE_SHADOW_DEBUGCOLOR
    colorData.rgb *= debugShadowColor * ao;
#else
    colorData.rgb *= shadow * ao;
#endif

#ifdef VERSE_GLES3
    fragData/*ColorBuffer*/ = colorData;
    dbgDepth/*DebugDepthBuffer*/ = vec4(debugValue, 1.0);
#else
    gl_FragData[0]/*ColorBuffer*/ = colorData;
    gl_FragData[1]/*DebugDepthBuffer*/ = vec4(debugValue, 1.0);
#endif
}
