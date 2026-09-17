uniform sampler2D ColorBuffer;
uniform sampler2D BloomBuffer, IblAmbientBuffer;
// Exposure of this frame, computed by the eye adaptation stage of the pipeline. It is a 1x1
// buffer holding the exposure encoded in log2 space, which keeps the bloom chain out of the
// exposure completely: BrightnessThreshold is free to be tuned for the bloom alone
uniform sampler2D ExposureBuffer;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

// Range of the encoding of the exposure, to keep in sync with std_exposure_adaptation.frag.glsl
const float ENCODE_MIN = -10.0, ENCODE_MAX = 6.0;

vec3 ReinhardToneMapping(vec3 color, float adapted_lum)
{
    const float MIDDLE_GREY = 1.0;
    color *= MIDDLE_GREY / adapted_lum;
    return color / (1.0 + color);
}

vec3 CEToneMapping(vec3 color, float adapted_lum)
{
    return 1.0 - exp(-adapted_lum * color);
}

vec3 Uncharted2HelperFunc(vec3 x)
{
    const float A = 0.22, B = 0.30, C = 0.10;
    const float D = 0.20, E = 0.01, F = 0.30;
    return ((x * (A * x + C * B) + vec3(D * E)) / (x * (A * x + B) + vec3(D * F))) - vec3(E / F);
}

vec3 Uncharted2ToneMapping(vec3 color, float adapted_lum)
{
    const float WHITE = 11.2;
    return Uncharted2HelperFunc(color * 1.6 * adapted_lum) / Uncharted2HelperFunc(vec3(WHITE));
}

vec3 ACESToneMapping(vec3 color, float adapted_lum)
{
    const float A = 2.51, B = 0.03, C = 2.43;
    const float D = 0.59, E = 0.14; color *= adapted_lum;
    return (color * (A * color + B)) / (color * (C * color + D) + vec3(E));
}

void main()
{
    vec2 uv0 = texCoord0.xy;
    vec4 color = VERSE_TEX2D(ColorBuffer, uv0), colorBloom = VERSE_TEX2D(BloomBuffer, uv0);
    vec4 iblColor = VERSE_TEX2D(IblAmbientBuffer, uv0);

    // The emissive term is not read here anymore: it is added to CombinedBuffer by the shadow
    // combining stage, so that it also takes part in the bloom extraction of this frame
    color.rgb = color.rgb + iblColor.rgb + colorBloom.rgb;
    float exposure = exp2(mix(ENCODE_MIN, ENCODE_MAX, VERSE_TEX2D(
        ExposureBuffer, vec2(0.5, 0.5)).r));
    color.rgb = ACESToneMapping(color.rgb, exposure);
    fragData = vec4(color.rgb, 1.0);
    VERSE_FS_FINAL(fragData);
}
