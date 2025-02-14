VERSE_VS_IN vec4 osg_Tangent;
VERSE_SRCIPT_DEF;
#ifdef VERSE_VRMODE
VERSE_VS_OUT vec4 texCoord0_gs, texCoord1_gs, color_gs;
VERSE_VS_OUT vec3 eyeNormal_gs, eyeTangent_gs, eyeBinormal_gs;
#else
VERSE_VS_OUT vec4 texCoord0, texCoord1, color;
VERSE_VS_OUT vec3 eyeNormal, eyeTangent, eyeBinormal;
#endif

void main()
{
#ifdef VERSE_VRMODE
    eyeNormal_gs = normalize(VERSE_MATRIX_N * osg_Normal);
    eyeTangent_gs = normalize(VERSE_MATRIX_N * osg_Tangent.xyz);
    eyeBinormal_gs = normalize(VERSE_MATRIX_N * (cross(osg_Normal, osg_Tangent.xyz) * osg_Tangent.w));
    texCoord0_gs = osg_MultiTexCoord0;
    texCoord1_gs = osg_MultiTexCoord1;
    color_gs = osg_Color;
    gl_Position = VERSE_MATRIX_MV * osg_Vertex;
#else
    eyeNormal = normalize(VERSE_MATRIX_N * osg_Normal);
    eyeTangent = normalize(VERSE_MATRIX_N * osg_Tangent.xyz);
    eyeBinormal = normalize(VERSE_MATRIX_N * (cross(osg_Normal, osg_Tangent.xyz) * osg_Tangent.w));
    texCoord0 = osg_MultiTexCoord0;
    texCoord1 = osg_MultiTexCoord1;
    color = osg_Color;
    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;
#endif
    VERSE_SCRIPT_FUNC(0);
}
