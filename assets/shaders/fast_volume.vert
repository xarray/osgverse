VERSE_VS_OUT vec4 eyeVertex, texCoord;

void main()
{
    texCoord = osg_MultiTexCoord0;
    eyeVertex = VERSE_MATRIX_MV * osg_Vertex;
    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;
}
