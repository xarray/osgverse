uniform sampler2D ColorBuffer;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

void main()
{
    vec2 uv0 = texCoord0.xy;
    vec4 color = VERSE_TEX2D(ColorBuffer, uv0);
    fragData = vec4(color.rgb, 1.0);
    VERSE_FS_FINAL(fragData);
}
