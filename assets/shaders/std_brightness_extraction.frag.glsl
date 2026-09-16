uniform sampler2D ColorBuffer;
uniform float BrightnessThreshold, BrightnessKnee;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

float luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    vec2 uv0 = texCoord0.xy;
    vec3 color = VERSE_TEX2D(ColorBuffer, uv0).rgb;
    float lum = luminance(color);

    // Soft knee extraction (same idea as UE's bloom): keep the original color instead of
    // turning everything into gray luminance. Set BrightnessKnee = 0 to get a hard cut.
    float soft = clamp(lum - BrightnessThreshold + BrightnessKnee, 0.0, 2.0 * BrightnessKnee);
    soft = soft * soft / (4.0 * BrightnessKnee + 0.00001);
    float contribution = max(soft, lum - BrightnessThreshold) / max(lum, 0.00001);
    fragData = vec4(color * contribution, 1.0);
    VERSE_FS_FINAL(fragData);
}
