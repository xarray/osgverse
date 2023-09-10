VERSE_VS_IN vec4 osg_Tangent;
VERSE_VS_OUT vec4 texCoord0, texCoord1, color, eyeVertex;
VERSE_VS_OUT vec3 eyeNormal, eyeTangent, eyeBinormal;

void main()
{
    eyeNormal = normalize(VERSE_MATRIX_N * osg_Normal);
    eyeTangent = normalize(VERSE_MATRIX_N * osg_Tangent);
    eyeBinormal = normalize(VERSE_MATRIX_N * (cross(osg_Normal, osg_Tangent.xyz) * osg_Tangent.w));
    eyeVertex = VERSE_MATRIX_MV * osg_Vertex;

    texCoord0 = osg_MultiTexCoord0;
    texCoord1 = osg_MultiTexCoord1;
    color = osg_Color;
    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;
}
