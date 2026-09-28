#pragma import_defines(VERSE_SHADOW_VSM, VERSE_SHADOW_ESM, VERSE_SHADOW_EVSM)
VERSE_FS_IN vec4 texCoord0, lightProjVec;
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


// ESM shape parameters, configured by ShadowModule::setShadowESMExponents():
//   .x  VSM / EVSM epsilon (unused by ESM)
//   .y  ESM over-darkening factor (unused, kept for symmetry with the combine stage)
//   .z  ESM depth scale: depth is multiplied by this before being negated in exp()
//   .w  reserved
// Default (0, 0.33, 15.0, 0) reproduces the original cast behavior (only .z matters for ESM)
uniform vec4 ShadowESMExponents;

// EVSM warp exponents: positive and negative depth scales used in WarpDepth() to build the
// warped moments stored in .r/.g (and .b/.a for the squared terms). Defaults (0.33, 15.0)
// reproduce the original cast behavior
uniform vec2 ShadowEVSMExponents;

#ifdef VERSE_SHADOW_EVSM
vec2 WarpDepth(in float depth, in vec2 exponents)
{
    float pos = exp(exponents.x * depth);
    float neg = -exp(-exponents.y * depth);
    return vec2(pos, neg);
}
#endif

void main()
{
#ifdef VERSE_SHADOW_VSM
    float dv = lightProjVec.z / lightProjVec.w;
    fragData = vec4(1.0, dv * dv, dv, 1.0);
    VERSE_FS_FINAL(fragData); return;
#endif

#ifdef VERSE_SHADOW_ESM
    float de = lightProjVec.z * 0.5 / lightProjVec.w + 0.5;
    fragData = vec4(1.0, 0.0, exp(-de * ShadowESMExponents.z), 1.0);
    VERSE_FS_FINAL(fragData); return;
#endif

#ifdef VERSE_SHADOW_EVSM
    float dev = lightProjVec.z * 0.5 / lightProjVec.w + 0.5;
    vec2 warpedDepth = WarpDepth(dev, ShadowEVSMExponents);
    fragData = vec4(warpedDepth.xy, warpedDepth.xy * warpedDepth.xy);
    VERSE_FS_FINAL(fragData); return;
#endif

#ifdef VERSE_WEBGL1
    fragData = EncodeFloatRGBA((lightProjVec.z * 0.5 / lightProjVec.w) + 0.5);
#else
    fragData = vec4(1.0, (lightProjVec.yz / lightProjVec.w), 1.0);
#endif
    VERSE_FS_FINAL(fragData);
}
