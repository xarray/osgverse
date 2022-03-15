#version 130
uniform mat4 osg_ViewMatrixInverse;
in vec3 osg_Tangent, osg_Binormal;
out vec4 worldVertex, texCoord0, texCoord1, color;
out vec3 eyeNormal, eyeTangent, eyeBinormal;

void main()
{
    mat4 modelMatrix = osg_ViewMatrixInverse * gl_ModelViewMatrix;
	eyeNormal = normalize(gl_NormalMatrix * gl_Normal);
    eyeTangent = normalize(gl_NormalMatrix * osg_Tangent);
    eyeBinormal = normalize(gl_NormalMatrix * osg_Binormal);
    
	worldVertex = modelMatrix * gl_Vertex;
	texCoord0 = gl_MultiTexCoord0;
	texCoord1 = gl_MultiTexCoord1;
    color = gl_Color;
	gl_Position = ftransform();
}
