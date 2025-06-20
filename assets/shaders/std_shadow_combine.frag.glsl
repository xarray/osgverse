#pragma import_defines(VERSE_SHADOW_EYESPACE)
uniform sampler2D ColorBuffer, SsaoBlurredBuffer, NormalBuffer, DepthBuffer;
uniform sampler2D ShadowMap0, ShadowMap1, ShadowMap2, ShadowMap3;
uniform sampler2D RandomTexture;
uniform mat4 ShadowSpaceMatrices[VERSE_MAX_SHADOWS];
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

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

float getShadowValue(in sampler2D shadowMap, in vec2 lightProjUV, in float depth)
{
    vec4 lightProjVec0 = VERSE_TEX2D(shadowMap, lightProjUV.xy);
#ifdef VERSE_WEBGL1
    float decDepth = DecodeFloatRGBA(lightProjVec0) * 2.0 - 1.0;
    float depth0 = decDepth;// lightProjVec0.z;  // use polygon-offset instead of +0.005
#else
    float depth0 = lightProjVec0.z;
#endif

#ifdef VERSE_SHADOW_EYESPACE
    return (depth < depth0) ? 0.0 : 1.0;
#else
    return (depth > depth0) ? 0.0 : 1.0;
#endif
}

float getShadowPCF_DirectionalLight(in sampler2D shadowMap, in vec2 lightProjUV, in float depth, in float uvRadius)
{
    float sum = 0.0;
    for (int i = 0; i < 16; i++)
    {
        vec2 dir = VERSE_TEX2D(RandomTexture, vec2(float(i) / 16.0, 0.25)).xy * 2.0 - vec2(1.0);
        sum += getShadowValue(shadowMap, lightProjUV.xy + dir * uvRadius, depth);
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
    float shadow = 1.0;
    for (int i = 0; i < VERSE_MAX_SHADOWS; ++i)
    {
        vec4 lightProjVec = ShadowSpaceMatrices[i] * eyeVertex;
        vec2 lightProjUV = (lightProjVec.xy / lightProjVec.w) * 0.5 + vec2(0.5);
        if (any(lessThan(lightProjUV, vec2(0.0))) || any(greaterThan(lightProjUV, vec2(1.0)))) continue;
        
        float depth = lightProjVec.z / lightProjVec.w;  // real depth in light space
        float shadowValue = 1.0, pcfRadius = 0.0012;
        
        if (i == 0) shadowValue = getShadowPCF_DirectionalLight(ShadowMap0, lightProjUV.xy, depth, pcfRadius);
        else if (i == 1) shadowValue = getShadowPCF_DirectionalLight(ShadowMap1, lightProjUV.xy, depth, pcfRadius);
        else if (i == 2) shadowValue = getShadowPCF_DirectionalLight(ShadowMap2, lightProjUV.xy, depth, pcfRadius);
        else if (i == 3) shadowValue = getShadowPCF_DirectionalLight(ShadowMap3, lightProjUV.xy, depth, pcfRadius);
        shadow *= shadowValue;
    }
    
    colorData.rgb *= shadow * ao;
    fragData = colorData;
    VERSE_FS_FINAL(fragData);
}
