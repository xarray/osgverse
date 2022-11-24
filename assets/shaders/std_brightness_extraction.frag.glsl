uniform sampler2D ColorBuffer;
uniform float BrightnessThreshold;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

float luminance(vec3 color)
{
    return dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    vec2 uv0 = texCoord0.xy;
    float lum = luminance(VERSE_TEX2D(ColorBuffer, uv0).xyz);
    fragData = vec4((lum > BrightnessThreshold) ? vec3(lum) : vec3(0.0), 1.0);
    VERSE_FS_FINAL(fragData);
}
