in vec3 osg_Tangent, osg_Binormal;
out vec4 texCoord0, texCoord1, color;
out vec3 eyeNormal, eyeTangent, eyeBinormal;

void main()
{
	eyeNormal = normalize(gl_NormalMatrix * gl_Normal);
    eyeTangent = normalize(gl_NormalMatrix * osg_Tangent);
    eyeBinormal = normalize(gl_NormalMatrix * osg_Binormal);
    
	texCoord0 = gl_MultiTexCoord0;
	texCoord1 = gl_MultiTexCoord1;
    color = gl_Color;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
