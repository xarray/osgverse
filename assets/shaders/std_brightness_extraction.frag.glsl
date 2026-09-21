uniform sampler2D ColorBuffer;
uniform float BrightnessThreshold, BrightnessKnee;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

float luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

// This pass is the entry point of the bloom chain, and the blur of the levels built from its
// result spreads every pixel over tens of screen pixels. A source which is not a number (a broken
// IBL texture, a division by zero somewhere in the lighting) would therefore not stay a single
// bad pixel: it would become a large black patch, as NaN survives every sum and multiplication
// and the tone mapping turns it into 0. The same happens with an infinite value, which the tone
// mapping also turns into a NaN. Those pixels get no bloom at all, and the highlights which are
// really there are bounded, so that a single absurd value can not dominate the whole bloom.
// The test can not rely on min() to filter NaN out: it is implemented as 'a < b ? a : b', which
// is false for a NaN and therefore returns the NaN
const float MAX_BLOOM_SOURCE = 100.0;

void main()
{
    vec2 uv0 = texCoord0.xy;
    vec3 color = VERSE_TEX2D(ColorBuffer, uv0).rgb;
    float lum = luminance(color);
    if (lum != lum)
    {
        fragData = vec4(0.0, 0.0, 0.0, 1.0);
        VERSE_FS_FINAL(fragData); return;
    }
    color = min(color, vec3(MAX_BLOOM_SOURCE));

    // Soft knee extraction (same idea as UE's bloom): keep the original color instead of
    // turning everything into gray luminance. Set BrightnessKnee = 0 to get a hard cut.
    float soft = clamp(lum - BrightnessThreshold + BrightnessKnee, 0.0, 2.0 * BrightnessKnee);
    soft = soft * soft / (4.0 * BrightnessKnee + 0.00001);
    float contribution = max(soft, lum - BrightnessThreshold) / max(lum, 0.00001);
    fragData = vec4(color * contribution, 1.0);
    VERSE_FS_FINAL(fragData);
}
