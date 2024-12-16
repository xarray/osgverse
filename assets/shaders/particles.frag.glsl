uniform sampler2D BaseTexture;
VERSE_FS_IN vec4 color, texCoord;
VERSE_FS_OUT vec4 fragData;

void main()
{
    vec4 baseColor = VERSE_TEX2D(BaseTexture, texCoord.st);
    fragData = baseColor * color;
    VERSE_FS_FINAL(fragData);
}
