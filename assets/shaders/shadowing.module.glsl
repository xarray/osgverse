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

float getDepthFromShadowMap(in sampler2D shadowMap, in vec2 lightProjUV)
{
    vec4 lightProjVec0 = VERSE_TEX2D(shadowMap, lightProjUV.xy);
#ifdef VERSE_WEBGL1
    float decDepth = DecodeFloatRGBA(lightProjVec0) * 2.0 - 1.0;
    return decDepth;// lightProjVec0.z;  // use polygon-offset instead of +0.005
#else
    return lightProjVec0.z;
#endif
}

float compareDepth(float depth0, float depth1)
{
#ifdef VERSE_SHADOW_EYESPACE
    return (depth1 < depth0) ? 0.0 : 1.0;
#else
    return (depth1 > depth0) ? 0.0 : 1.0;
#endif
}

vec3 getShadowValue(in sampler2D shadowMap, in vec2 lightProjUV, in float depth)
{
    float depth0 = getDepthFromShadowMap(shadowMap, lightProjUV);
    return vec3(depth0, depth, compareDepth(depth0, depth));
}

vec3 getShadowValue_PossionPCF(in sampler2D shadowMap, in sampler2D randomMap, in vec2 lightProjUV,
                               in float depth, in vec2 invMapSize)
{
    vec2 shadowUVbiased = lightProjUV, fractCoord = 1.0 - fract(lightProjUV);
#ifdef GL_OES_standard_derivatives
    shadowUVbiased.x += dFdx(lightProjUV.xy).x * invMapSize.x;
    shadowUVbiased.y += dFdy(lightProjUV.xy).y * invMapSize.y;
#endif

    float shadowed = 0.0, inv = 1.0 / 16.0, radius = length(invMapSize) * 2.0;
    for (int i = 0; i < 16; i++)
    {
        vec2 dir = VERSE_TEX2D(randomMap, vec2(float(i) * inv, 0.25)).xy * 2.0 - vec2(1.0);
        shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + dir * vec2(radius)), depth);
    }
    return vec3(0.0, depth, shadowed * inv);
}

vec3 getShadowValue_BandPCF(in sampler2D shadowMap, in vec2 lightProjUV, in float depth, in vec2 invMapSize)
{
    vec2 shadowUVbiased = lightProjUV, fractCoord = 1.0 - fract(lightProjUV);
#ifdef GL_OES_standard_derivatives
    shadowUVbiased.x += dFdx(lightProjUV.xy).x * invMapSize.x;
    shadowUVbiased.y += dFdy(lightProjUV.xy).y * invMapSize.y;
#endif

    float dx0 = -invMapSize.x, dy0 = -invMapSize.y, dx1 = invMapSize.x, dy1 = invMapSize.y;
    float dx2 = -2.0 * invMapSize.x, dy2 = -2.0 * invMapSize.y;
    float dx3 = 2.0 * invMapSize.x, dy3 = 2.0 * invMapSize.y, shadowed = 0.0;
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx2, dy2)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx0, dy2)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx1, dy2)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx3, dy2)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx2, dy0)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx0, dy0)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx1, dy0)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx3, dy0)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx2, dy1)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx0, dy1)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx1, dy1)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx3, dy1)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx2, dy3)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx0, dy3)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx1, dy3)), depth);
    shadowed += compareDepth(getDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx3, dy3)), depth);
    return vec3(0.0, depth, shadowed / 16.0);
}
