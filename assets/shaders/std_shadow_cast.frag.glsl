VERSE_FS_IN vec4 texCoord0, lightProjVec;
VERSE_FS_OUT vec4 fragData;

void main()
{
    fragData = vec4(1.0, (lightProjVec.yz / lightProjVec.w), 1.0);
    VERSE_FS_FINAL(fragData);
}
