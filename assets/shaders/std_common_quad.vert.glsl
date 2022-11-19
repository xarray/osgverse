out vec4 texCoord0;

void main()
{
	texCoord0 = gl_MultiTexCoord0;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
