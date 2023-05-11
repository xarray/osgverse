VERSE_VS_IN vec3 osg_Tangent, osg_Binormal;
VERSE_VS_OUT vec4 texCoord0, texCoord1, color;
VERSE_VS_OUT vec3 eyeNormal, eyeTangent, eyeBinormal;

void main()
{
    eyeNormal = normalize(VERSE_MATRIX_N * osg_Normal);
    eyeTangent = normalize(VERSE_MATRIX_N * osg_Tangent);
    eyeBinormal = normalize(VERSE_MATRIX_N * osg_Binormal);

    texCoord0 = osg_MultiTexCoord0;
    texCoord1 = osg_MultiTexCoord1;
    color = osg_Color;
    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;
}
