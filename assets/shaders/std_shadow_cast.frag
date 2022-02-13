#version 130
in vec4 gl_TexCoord[gl_MaxTextureCoords];
in vec4 lightProjVec;

void main()
{
	gl_FragData[0] = vec4(1.0, (lightProjVec.yz / lightProjVec.w), 1.0);
}
