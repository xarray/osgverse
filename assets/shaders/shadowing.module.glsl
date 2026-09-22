const vec4 bitEnc = vec4(1., 255., 65025., 16581375.);
const vec4 bitDec = 1. / bitEnc;

// Receiver-side bias of the shadow lookup, configured by ShadowModule::setShadowBias().
// All three values are multiples of one shadow-map texel, so the bias follows the world size
// of a texel and does not have to be retuned when the cascade resolution or depth range
// changes (a bias expressed in normalized depth would grow with the depth range of a cascade,
// which is what erases the small shadows of the far cascades):
//   .x  constant part
//   .y  slope scale, multiplied by the depth slope of the receiver
//   .z  normal offset, applied to the lookup position along the receiver normal
uniform vec4 ShadowBiasParams;

// Depth delta of one shadow-map texel of each cascade, in the same space as CompareDepth()
uniform vec4 ShadowBiasScales;

// World size of one shadow-map texel of each cascade, used by the normal offset
uniform vec4 ShadowTexelSizes;

// World-space direction from a surface to the main light, taken from the LightDrawable as it
// is. The shaders convert it to eye space with the same G-Buffer matrix they use for every
// other transform, so the CPU and the GPU can not disagree about the matrix convention
uniform vec3 MainLightDirection;

vec3 getEyeSpaceLightDirection(in mat4 worldToView)
{
    return normalize((worldToView * vec4(MainLightDirection, 0.0)).xyz);
}

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

float CompareDepth(float depth0, float depth1, float bias)
{
#ifdef VERSE_SHADOW_EYESPACE
    return (depth1 < (depth0 - bias)) ? 0.0 : 1.0;
#else
    return (depth1 > (depth0 + bias)) ? 0.0 : 1.0;
#endif
}

// Slope-scaled depth bias of the receiver, in the depth space compared by CompareDepth().
// A single constant has to be large enough for the worst grazing angle, which is exactly what
// makes the shadow detach from its caster (peter-panning); scaling the extra part by the depth
// slope keeps the bias small on surfaces facing the light. The two parts are added instead of
// taking their maximum, so that the bias stays continuous over a curved surface and does not
// leave a seam in the shadow boundary where the two would cross
float getShadowDepthBias(in vec3 eyeNormal, in vec3 lightDir, in float texelDepthDelta)
{
    float cosT = clamp(dot(eyeNormal, lightDir), 0.0, 1.0);
    // The slope is capped: a surface almost parallel to the light receives almost no direct
    // light, so letting the bias grow without limit there only removes valid shadows
    float tanT = min(sqrt(max(1.0 - cosT * cosT, 0.0)) / max(cosT, 1e-3), 2.0);
    return (ShadowBiasParams.x + ShadowBiasParams.y * tanT) * texelDepthDelta;
}

// Per-cascade accessors. The components are picked with a chain of constant index reads
// instead of a dynamic array index, which GLES2 does not allow on uniforms
float getShadowBiasScale(in int cascade)
{
    if (cascade == 0) return ShadowBiasScales.x;
    else if (cascade == 1) return ShadowBiasScales.y;
    else if (cascade == 2) return ShadowBiasScales.z;
    return ShadowBiasScales.w;
}

float getShadowTexelSize(in int cascade)
{
    if (cascade == 0) return ShadowTexelSizes.x;
    else if (cascade == 1) return ShadowTexelSizes.y;
    else if (cascade == 2) return ShadowTexelSizes.z;
    return ShadowTexelSizes.w;
}

// Move the lookup position along the receiver normal before projecting it into light space.
// The view matrix is a rigid transform, so an offset applied in eye space keeps the length of
// the wanted world-space one. This removes the acne without adding peter-panning, because it
// changes where the shadow map is read instead of how deep the receiver is assumed to be
vec4 getShadowLookupVertex(in vec4 eyeVertex, in vec3 eyeNormal, in float worldTexelSize)
{
    vec4 v = eyeVertex;
    v.xyz += eyeNormal * (ShadowBiasParams.z * worldTexelSize);
    return v;
}

vec3 getShadowValue(in sampler2D shadowMap, in vec2 lightProjUV, in float depth, in float bias)
{
    float depth0 = GetDepthFromShadowMap(shadowMap, lightProjUV);
    return vec3(depth0, depth, CompareDepth(depth0, depth, bias));
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
                               in float depth, in vec2 invMapSize, in float bias)
{
    float shadowed = 0.0, inv = 1.0 / 16.0, radius = length(invMapSize) * 2.0;
    for (int i = 0; i < 16; i++)
    {
        vec2 dir = VERSE_TEX2D(randomMap, vec2(float(i) * inv, 0.25)).xy * 2.0 - vec2(1.0);
        shadowed += CompareDepth(
            GetDepthFromShadowMap(shadowMap, lightProjUV + dir * vec2(radius)), depth, bias);
    }
    return vec3(0.0, depth, shadowed * inv);
}

vec3 getShadowValue_BandPCF(in sampler2D shadowMap, in vec2 lightProjUV, in float depth,
                            in vec2 invMapSize, in float bias)
{
    vec2 shadowUVbiased = lightProjUV;
    float dx0 = -invMapSize.x, dy0 = -invMapSize.y, dx1 = invMapSize.x, dy1 = invMapSize.y;
    float dx2 = -2.0 * invMapSize.x, dy2 = -2.0 * invMapSize.y;
    float dx3 = 2.0 * invMapSize.x, dy3 = 2.0 * invMapSize.y, shadowed = 0.0;
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx2, dy2)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx0, dy2)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx1, dy2)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx3, dy2)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx2, dy0)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx0, dy0)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx1, dy0)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx3, dy0)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx2, dy1)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx0, dy1)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx1, dy1)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx3, dy1)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx2, dy3)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx0, dy3)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx1, dy3)), depth, bias);
    shadowed += CompareDepth(GetDepthFromShadowMap(shadowMap, shadowUVbiased + vec2(dx3, dy3)), depth, bias);
    return vec3(0.0, depth, shadowed / 16.0);
}

// Screen-space contact shadows. A shadow map can not resolve the gap at a contact point: one of
// its texels is a length in world space, so the darkening which should appear where an object
// touches the ground is simply missing from it, whatever the receiver-side bias does. A short ray
// marched towards the light recovers that occlusion, and since it is tested against the depth
// buffer its precision follows the screen resolution instead of the shadow map.
// The matrices are passed in instead of being taken from the shader which includes this module,
// so that the module keeps working in any pass that has them under another name
//   .x  ray length in world units (<= 0 disables the effect)
//   .y  thickness: how far behind the depth buffer a sample may be and still be an occlusion
//   .z  bias: offset of the ray start along the surface normal, and the depth difference a sample
//       needs before it is considered an occluder
//   .w  strength of the recovered occlusion
uniform vec4 ContactShadowParams;

float getContactShadowValue(in sampler2D depthBuffer, in mat4 viewToProj, in mat4 projToView,
                            in vec3 eyePos, in vec3 eyeNormal, in vec3 eyeLightDir)
{
    if (ContactShadowParams.x <= 0.0 || ContactShadowParams.w <= 0.0) return 1.0;

    // A surface facing away from the light is not lit at all, so there is no contact shadow to
    // recover for it. A nearly grazing light is excluded as well: the ray would then run along
    // the surface, where every step hits the surface itself
    if (dot(eyeNormal, eyeLightDir) <= 0.01) return 1.0;

    const int CONTACT_SHADOW_STEPS = 12;
    float rayLength = ContactShadowParams.x;
    float stepLength = rayLength / float(CONTACT_SHADOW_STEPS);
    // The ray starts a little above the surface, otherwise the first samples are blocked by the
    // surface they leave from
    vec3 rayStart = eyePos + eyeNormal * ContactShadowParams.z;

    // Dither the step positions per pixel: the fixed number of steps would otherwise show as
    // bands, while dithered noise is what the following anti-aliasing stage can average away
    float dither = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
    float occlusion = 0.0;
    for (int i = 0; i < CONTACT_SHADOW_STEPS; ++i)
    {
        float t = (float(i) + dither) * stepLength;
        vec3 samplePos = rayStart + eyeLightDir * t;
        vec4 clip = viewToProj * vec4(samplePos, 1.0);
        if (clip.w <= 0.0) break;  // marched behind the camera: no more useful samples

        vec2 sampleUV = (clip.xy / clip.w) * 0.5 + vec2(0.5);
        if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
            break;  // left the screen: the rest of the ray is not visible either

        float sceneDepth = VERSE_TEX2D(depthBuffer, sampleUV).r;
        if (sceneDepth >= 1.0) continue;  // background: nothing which could block the ray

        vec4 sceneProj = projToView *
            vec4(sampleUV * 2.0 - vec2(1.0), sceneDepth * 2.0 - 1.0, 1.0);
        // Both values are distances in front of the camera, so a positive difference means the
        // sample is behind the geometry of that pixel, i.e. the ray is blocked there
        float sceneViewZ = -(sceneProj.z / sceneProj.w), sampleViewZ = -samplePos.z;
        float difference = sampleViewZ - sceneViewZ;
        if (difference > ContactShadowParams.z && difference < ContactShadowParams.y)
            occlusion = max(occlusion, 1.0 - t / rayLength);  // a nearby hit occludes more
    }

    if (occlusion <= 0.0) return 1.0;
    // Fade the whole effect out as the light direction approaches the view direction: the ray
    // then spans almost no screen space and comparing its samples is meaningless
    float viewFade = 1.0 - smoothstep(0.9, 0.99, abs(dot(eyeLightDir, normalize(-eyePos))));
    return 1.0 - clamp(occlusion * ContactShadowParams.w * viewFade, 0.0, 1.0);
}
