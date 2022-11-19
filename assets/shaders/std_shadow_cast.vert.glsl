out vec4 texCoord0, lightProjVec;

void main()
{
	lightProjVec = gl_ModelViewProjectionMatrix * gl_Vertex;
	texCoord0 = gl_MultiTexCoord0;
	gl_Position = lightProjVec;
}
