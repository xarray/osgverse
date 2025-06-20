#pragma import_defines(VERSE_SHADOW_EYESPACE, VERSE_SHADOW_DEBUGCOLOR)
uniform sampler2D ColorBuffer, SsaoBlurredBuffer, NormalBuffer, DepthBuffer;
uniform sampler2D ShadowMap0, ShadowMap1, ShadowMap2, ShadowMap3;
uniform sampler2D RandomTexture;
uniform mat4 ShadowSpaceMatrices[VERSE_MAX_SHADOWS];
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
VERSE_FS_IN vec4 texCoord0;

#ifdef VERSE_GLES3
layout(location = 0) VERSE_FS_OUT vec4 fragData;
layout(location = 1) VERSE_FS_OUT vec4 dbgDepth;
#endif

#ifdef VERSE_WEBGL1
const vec4 bitEnc = vec4(1., 255., 65025., 16581375.);
const vec4 bitDec = 1. / bitEnc;

vec4 EncodeFloatRGBA(float v)
{
    vec4 enc = fract(bitEnc * v);
    enc -= enc.yzww * vec2(1. / 255., 0.).xxxy;
    return enc;
}

float DecodeFloatRGBA(vec4 v)
{
    v = floor(v * 255.0 + 0.5) / 255.0;
    return dot(v, bitDec);
}
#endif

vec3 getShadowValue(in sampler2D shadowMap, in vec2 lightProjUV, in float depth)
{
    vec4 lightProjVec0 = VERSE_TEX2D(shadowMap, lightProjUV.xy);
#ifdef VERSE_WEBGL1
    float decDepth = DecodeFloatRGBA(lightProjVec0) * 2.0 - 1.0;
    float depth0 = decDepth;// lightProjVec0.z;  // use polygon-offset instead of +0.005
#else
    float depth0 = lightProjVec0.z;
#endif

#ifdef VERSE_SHADOW_EYESPACE
    return vec3(depth0, depth, (depth < depth0) ? 0.0 : 1.0);
#else
    return vec3(depth0, depth, (depth > depth0) ? 0.0 : 1.0);
#endif
}

float getShadowPCF_DirectionalLight(in sampler2D shadowMap, in vec2 lightProjUV, in float depth, in float uvRadius)
{
    float sum = 0.0;
    for (int i = 0; i < 16; i++)
    {
        vec2 dir = VERSE_TEX2D(RandomTexture, vec2(float(i) / 16.0, 0.25)).xy * 2.0 - vec2(1.0);
        sum += getShadowValue(shadowMap, lightProjUV.xy + dir * uvRadius, depth).z;
    }
    return sum / 16.0;
}

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
        if (i == 0) shadowValue = getShadowValue(ShadowMap0, lightProjUV.xy, depth);
        else if (i == 1) shadowValue = getShadowValue(ShadowMap1, lightProjUV.xy, depth);
        else if (i == 2) shadowValue = getShadowValue(ShadowMap2, lightProjUV.xy, depth);
        else if (i == 3) shadowValue = getShadowValue(ShadowMap3, lightProjUV.xy, depth);

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
