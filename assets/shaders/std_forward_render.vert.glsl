#pragma import_defines(VERSE_VRMODE)
VERSE_VS_IN vec4 osg_Tangent;
#ifdef VERSE_VRMODE
VERSE_VS_OUT vec4 texCoord0_gs, texCoord1_gs, color_gs, eyeVertex_gs;
VERSE_VS_OUT vec3 eyeNormal_gs, eyeTangent_gs, eyeBinormal_gs;
#else
VERSE_VS_OUT vec4 texCoord0, texCoord1, color, eyeVertex;
VERSE_VS_OUT vec3 eyeNormal, eyeTangent, eyeBinormal;
#endif
uniform mat4 osg_ViewMatrixInverse;

void main()
{
#ifdef VERSE_VRMODE
    eyeNormal_gs = normalize(VERSE_MATRIX_N * osg_Normal);
    eyeTangent_gs = normalize(VERSE_MATRIX_N * osg_Tangent.xyz);
    eyeBinormal_gs = normalize(VERSE_MATRIX_N * (cross(osg_Normal, osg_Tangent.xyz) * osg_Tangent.w));
    eyeVertex_gs = osg_ViewMatrixInverse * VERSE_MATRIX_MV * osg_Vertex;

    texCoord0_gs = osg_MultiTexCoord0;
    texCoord1_gs = osg_MultiTexCoord1;
    color_gs = osg_Color;
    gl_Position = eyeVertex_gs;
#else
    eyeNormal = normalize(VERSE_MATRIX_N * osg_Normal);
    eyeTangent = normalize(VERSE_MATRIX_N * osg_Tangent.xyz);
    eyeBinormal = normalize(VERSE_MATRIX_N * (cross(osg_Normal, osg_Tangent.xyz) * osg_Tangent.w));
    eyeVertex = VERSE_MATRIX_MV * osg_Vertex;

    texCoord0 = osg_MultiTexCoord0;
    texCoord1 = osg_MultiTexCoord1;
    color = osg_Color;
    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;
#endif
}
