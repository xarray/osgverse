VERSE_VS_OUT vec4 texCoord0;

void main()
{
	texCoord0 = gl_MultiTexCoord0;
	gl_Position = VERSE_MATRIX_MVP * gl_Vertex;
}
