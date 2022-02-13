#version 130
out vec4 gl_TexCoord[gl_MaxTextureCoords];
out vec4 lightProjVec;

void main()
{
	lightProjVec = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
	gl_FrontColor = gl_Color;
	gl_BackColor = gl_Color;
}
