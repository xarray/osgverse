uniform sampler2D BaseTexture;
uniform vec4 TextureSheetRange;
uniform vec2 TextureSheetTiles;
VERSE_FS_IN vec4 color, texCoord;
VERSE_FS_IN float animationID, lifeTime;
VERSE_FS_OUT vec4 fragData;

void main()
{
    vec2 uv = texCoord.st; if (lifeTime < 0.001) discard;
    if (TextureSheetTiles.x > 1.0 || TextureSheetTiles.y > 1.0)
    {
        float r = float(int(animationID)) / TextureSheetTiles.x;
        float c = floor(r) / TextureSheetTiles.x; r = fract(r);
        uv = uv / TextureSheetTiles + vec2(r, c);
    }

    vec4 baseColor = VERSE_TEX2D(BaseTexture, vec2(uv.x, 1.0 - uv.y));
    fragData = baseColor * color;
    VERSE_FS_FINAL(fragData);
}
