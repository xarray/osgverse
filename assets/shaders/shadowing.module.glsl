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

vec2 WarpDepth(in float depth, in vec2 exponents)
{
    float pos = exp(exponents.x * depth);
    float neg = -exp(-exponents.y * depth);
    return vec2(pos, neg);
}

float ChebychevInequality(const in vec2 moments, const in float t)
{
    // Calculate variance, which is actually the amount of
    // error due to precision loss from fp32 to RG/BA (moment1 / moment2)
    if (t > moments.x)
    {
        float variance = moments.y - (moments.x * moments.x);
        variance = max(variance, 0.02);

        float d = t - moments.x;  // Calculate the upper bound
        return variance / (variance + d * d);
    }
    return 1.0;  // No shadow if depth of fragment is in front
}

float ChebyshevUpperBound(const in vec2 moments, const in float mean, const in float minVariance)
{
    // http://http.developer.nvidia.com/GPUGems3/gpugems3_ch08.html
    float d = mean - moments.x;
    if (d > 0.0)
    {
        float variance = moments.y - (moments.x * moments.x);
        variance = max(variance, minVariance);  // Compute variance

        // Compute probabilistic upper bound: p represent an upper bound on the visibility percentage of the receiver.
        // Remove the [0, Amount] tail and linearly rescale (Amount, 1]. light bleeding when shadows overlap.
        float p = smoothstep(mean, mean, moments.x);
        float pMax = smoothstep(0.2, 1.0, variance / (variance + d * d));
        return clamp(max(p, pMax), 0.0, 1.0);  // One-tailed chebyshev
    }
    return 1.0;
}

float GetDepthFromShadowMap(in sampler2D shadowMap, in vec2 lightProjUV)
{
    vec4 lightProjVec0 = VERSE_TEX2D(shadowMap, lightProjUV.xy);
#ifdef VERSE_WEBGL1
    float decDepth = DecodeFloatRGBA(lightProjVec0) * 2.0 - 1.0;
    return decDepth;// lightProjVec0.z;  // use polygon-offset instead of +0.005
#else
    return lightProjVec0.z;
#endif
}

float CompareDepth(float depth0, float depth1)
{
#ifdef VERSE_SHADOW_EYESPACE
    return (depth1 < depth0) ? 0.0 : 1.0;
#else
    return (depth1 > depth0) ? 0.0 : 1.0;
#endif
}

vec3 getShadowValue(in sampler2D shadowMap, in vec2 lightProjUV, in float depth)
{
    float depth0 = GetDepthFromShadowMap(shadowMap, lightProjUV);
    return vec3(depth0, depth, CompareDepth(depth0, depth));
}

vec3 getShadowValue_VSM(in sampler2D shadowMap, in vec2 lightProjUV, in float depth, in float epsilonVSM)
{
    // Copied from osgjs/sources/osgShadow/shaders/vsm.glsl
    vec2 moments = VERSE_TEX2D(shadowMap, lightProjUV.xy).zy;
    return vec3(moments.x, depth, ChebyshevUpperBound(moments, depth, epsilonVSM));
}

vec3 getShadowValue_ESM(in sampler2D shadowMap, in vec2 lightProjUV, in float depth,
                        in float exponent0, in float exponent1)
{
    // Copied from osgjs/sources/osgShadow/shaders/esm.glsl
    float occluder = GetDepthFromShadowMap(shadowMap, lightProjUV);
    float over_darkening_factor = exponent0, depthScale = exponent1;
    float receiver = occluder * exp(depthScale * (depth * 0.5 + 0.5));
    return vec3(occluder, depth, 1.0 - clamp(over_darkening_factor * receiver, 0.0, 1.0));
}

vec3 getShadowValue_EVSM(in sampler2D shadowMap, in vec2 lightProjUV, in float depth,
                         in float exponent0, in float exponent1, in float epsilonVSM)
{
    // Copied from osgjs/sources/osgShadow/shaders/evsm.glsl
    vec4 occluder = VERSE_TEX2D(shadowMap, lightProjUV.xy);
    vec2 exponents = vec2(exponent0, exponent1);
    vec2 warpedDepth = WarpDepth(depth * 0.5 + 0.5, exponents);

    float derivationEVSM = epsilonVSM;  // Derivative of warping at depth
    vec2 depthScale = vec2(derivationEVSM) * exponents * warpedDepth;
    vec2 minVariance = depthScale * depthScale;

    // Compute the upper bounds of the visibility function both for x and y
    float posContrib = ChebyshevUpperBound(occluder.xz, -warpedDepth.x, minVariance.x);
    float negContrib = ChebyshevUpperBound(occluder.yw, warpedDepth.y, minVariance.y);
    return vec3(posContrib, negContrib, min(posContrib, negContrib));
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
        shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + dir * vec2(radius)), depth);
    }
    return vec3(0.0, depth, shadowed * inv);
}

vec3 getShadowValue_BandPCF(in sampler2D shadowMap, in vec2 lightProjUV, in float depth, in vec2 invMapSize)
{
    // Copied from osgjs/sources/osgShadow/shaders/bandPCF.glsl
    vec2 shadowUVbiased = lightProjUV, fractCoord = 1.0 - fract(lightProjUV);
#ifdef GL_OES_standard_derivatives
    shadowUVbiased.x += dFdx(lightProjUV.xy).x * invMapSize.x;
    shadowUVbiased.y += dFdy(lightProjUV.xy).y * invMapSize.y;
#endif

    float dx0 = -invMapSize.x, dy0 = -invMapSize.y, dx1 = invMapSize.x, dy1 = invMapSize.y;
    float dx2 = -2.0 * invMapSize.x, dy2 = -2.0 * invMapSize.y;
    float dx3 = 2.0 * invMapSize.x, dy3 = 2.0 * invMapSize.y, shadowed = 0.0;
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx2, dy2)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx0, dy2)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx1, dy2)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx3, dy2)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx2, dy0)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx0, dy0)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx1, dy0)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx3, dy0)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx2, dy1)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx0, dy1)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx1, dy1)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx3, dy1)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx2, dy3)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx0, dy3)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx1, dy3)), depth);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx3, dy3)), depth);
    return vec3(0.0, depth, shadowed / 16.0);
}
