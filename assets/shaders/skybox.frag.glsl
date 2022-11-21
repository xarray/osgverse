#if VERSE_CUBEMAP_SKYBOX
uniform samplerCube SkyTexture;
VERSE_FS_IN vec3 texCoord;
VERSE_FS_OUT vec4 fragData;

void main()
{
    vec4 skyColor = VERSE_TEXCUBE(SkyTexture, texCoord);
    fragData = pow(skyColor, vec4(1.0 / 2.2));
    VERSE_FS_FINAL(fragData);
}
#else
uniform sampler2D SkyTexture;
VERSE_FS_IN vec3 texCoord;
VERSE_FS_OUT vec4 fragData;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 sphericalUV(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan; uv += vec2(0.5);
    return clamp(uv, vec2(0.0), vec2(1.0));
}

void main()
{
    vec4 skyColor = VERSE_TEX2D(SkyTexture, sphericalUV(texCoord));
    fragData = pow(skyColor, vec4(1.0 / 2.2));
    VERSE_FS_FINAL(fragData);
}
#endif
