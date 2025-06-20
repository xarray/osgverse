#pragma import_defines(VERSE_SHADOW_EYESPACE)
VERSE_SRCIPT_DEF
#ifdef VERSE_VRMODE
VERSE_VS_OUT vec4 texCoord0_gs, lightProjVec_gs;
#else
VERSE_VS_OUT vec4 texCoord0, lightProjVec;
#endif

void main()
{
#ifdef VERSE_VRMODE
    texCoord0_gs = osg_MultiTexCoord0;
    lightProjVec_gs = VERSE_MATRIX_MV * osg_Vertex;
    gl_Position = lightProjVec_gs;
#else
    texCoord0 = osg_MultiTexCoord0;
#  ifdef VERSE_SHADOW_EYESPACE
    lightProjVec = VERSE_MATRIX_MV * osg_Vertex;
    gl_Position = VERSE_MATRIX_P * lightProjVec;
#  else
    lightProjVec = VERSE_MATRIX_MVP * osg_Vertex;
    gl_Position = lightProjVec;
#  endif
#endif
}
