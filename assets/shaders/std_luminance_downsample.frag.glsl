uniform sampler2D ColorBuffer;
uniform vec2 InvBufferResolution;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

void main()
{
	vec2 uv0 = texCoord0.xy, texelSize = InvBufferResolution;
    vec4 color = VERSE_TEX2D(ColorBuffer, uv0);
    color += VERSE_TEX2D(ColorBuffer, uv0 + vec2(texelSize.x, 0.0));
    color += VERSE_TEX2D(ColorBuffer, uv0 + vec2(0.0, texelSize.y));
    color += VERSE_TEX2D(ColorBuffer, uv0 + vec2(texelSize.x, texelSize.y));
	fragData = vec4(color.rgb * 0.25, 1.0);
    VERSE_FS_FINAL(fragData);
}
