uniform sampler2D ColorBuffer, LuminanceBuffer;
uniform sampler2D BloomBuffer, IblAmbientBuffer;
uniform vec2 LuminanceFactor;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

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
    vec4 color = VERSE_TEX2D(ColorBuffer, uv0);
    vec4 colorBloom = VERSE_TEX2D(BloomBuffer, uv0);
    vec4 iblColor = VERSE_TEX2D(IblAmbientBuffer, uv0);
    float lumAvg = VERSE_TEX2D(LuminanceBuffer, vec2(0.5, 0.5)).r;

    color.rgb = color.rgb + iblColor.rgb + colorBloom.rgb;
    if (true)
        color.rgb = ACESToneMapping(color.rgb, LuminanceFactor.x + lumAvg * LuminanceFactor.y);
    fragData = vec4(color.rgb, 1.0);
    VERSE_FS_FINAL(fragData);
}
