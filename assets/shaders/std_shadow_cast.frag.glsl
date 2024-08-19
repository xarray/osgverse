VERSE_FS_IN vec4 texCoord0, lightProjVec;
VERSE_FS_OUT vec4 fragData;

#if VERSE_WEBGL1
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

void main()
{
#if VERSE_WEBGL1
    fragData = EncodeFloatRGBA((lightProjVec.z * 0.5 / lightProjVec.w) + 0.5);
#else
    fragData = vec4(1.0, (lightProjVec.yz / lightProjVec.w), 1.0);
#endif
    VERSE_FS_FINAL(fragData);
}
