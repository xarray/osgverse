uniform sampler2D BrightnessBuffer1, BrightnessBuffer2;
uniform sampler2D BrightnessBuffer3, BrightnessBuffer4;
uniform vec4 BloomWeights;
uniform vec2 InvScreenResolution;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

vec4 blur5(in sampler2D image, in vec2 uv)
{
    vec4 color = vec4(0.0); vec2 dir = vec2(1.0, 0.0);
    vec2 off1 = vec2(1.3333333333333333) * dir * InvScreenResolution;
    color += VERSE_TEX2D(image, uv) * 0.29411764705882354 * 2.0;
    color += VERSE_TEX2D(image, uv + off1) * 0.35294117647058826;
    color += VERSE_TEX2D(image, uv - off1) * 0.35294117647058826;

    dir = vec2(0.0, 1.0);
    off1 = vec2(1.3333333333333333) * dir * InvScreenResolution;
    color += VERSE_TEX2D(image, uv + off1) * 0.35294117647058826;
    color += VERSE_TEX2D(image, uv - off1) * 0.35294117647058826;
    return color * 0.5;
}

void main()
{
    vec2 uv0 = texCoord0.xy;
    vec4 color1 = blur5(BrightnessBuffer1, uv0);
    vec4 color2 = blur5(BrightnessBuffer2, uv0);
    vec4 color3 = blur5(BrightnessBuffer3, uv0);
    vec4 color4 = blur5(BrightnessBuffer4, uv0);

    // Combine all resolution levels with explicit weights. The brightness buffers are RGB
    // textures without alpha, so the old 'mix(color, next, next.a)' way would always take
    // the coarsest level and lose the multi-scale structure. Weights are normalized here,
    // so BloomWeights can be tweaked freely without changing the overall bloom strength.
    vec4 weights = BloomWeights / max(BloomWeights.x + BloomWeights.y +
                                      BloomWeights.z + BloomWeights.w, 0.00001);
    vec3 color = color1.rgb * weights.x + color2.rgb * weights.y
               + color3.rgb * weights.z + color4.rgb * weights.w;
    fragData = vec4(color, 1.0);
    VERSE_FS_FINAL(fragData);
}
