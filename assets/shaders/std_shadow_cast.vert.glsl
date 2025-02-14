VERSE_SRCIPT_DEF;
#ifdef VERSE_VRMODE
VERSE_VS_OUT vec4 texCoord0_gs, lightProjVec_gs;
#else
VERSE_VS_OUT vec4 texCoord0, lightProjVec;
#endif

void main()
{
#ifdef VERSE_VRMODE
    lightProjVec_gs = VERSE_MATRIX_MV * osg_Vertex;
    texCoord0_gs = osg_MultiTexCoord0;
    gl_Position = lightProjVec_gs;
#else
    lightProjVec = VERSE_MATRIX_MVP * osg_Vertex;
    texCoord0 = osg_MultiTexCoord0;
    gl_Position = lightProjVec;
#endif
}
