VERSE_VS_OUT vec4 texCoord0;

void main()
{
    texCoord0 = osg_MultiTexCoord0;
    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;
}
