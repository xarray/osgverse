VERSE_VS_IN vec3 osg_Tangent, osg_Binormal;
VERSE_VS_OUT vec4 texCoord0_gs, texCoord1_gs, color_gs;
VERSE_VS_OUT vec3 eyeNormal_gs, eyeTangent_gs, eyeBinormal_gs;

void main()
{
    eyeNormal_gs = normalize(VERSE_MATRIX_N * osg_Normal);
    eyeTangent_gs = normalize(VERSE_MATRIX_N * osg_Tangent);
    eyeBinormal_gs = normalize(VERSE_MATRIX_N * osg_Binormal);
    
    texCoord0_gs = osg_MultiTexCoord0;
    texCoord1_gs = osg_MultiTexCoord1;
    color_gs = osg_Color;
    gl_Position = VERSE_MATRIX_MV * osg_Vertex;
}
