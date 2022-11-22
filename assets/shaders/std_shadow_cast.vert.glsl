VERSE_VS_OUT vec4 texCoord0, lightProjVec;

void main()
{
	lightProjVec = VERSE_MATRIX_MVP * osg_Vertex;
	texCoord0 = osg_MultiTexCoord0;
	gl_Position = lightProjVec;
}
