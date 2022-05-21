#version 130
uniform mat4 osg_ViewMatrixInverse;
in vec3 osg_Tangent;
out vec4 eyeVertex, texCoord0, texCoord1, texCoord2;
out vec3 eyeNormal, eyeTangent, eyeBinormal;

void main()
{
	eyeVertex = gl_ModelViewMatrix * gl_Vertex;
	eyeNormal = normalize(gl_NormalMatrix * gl_Normal);
    eyeTangent = normalize(gl_NormalMatrix * osg_Tangent);
    eyeBinormal = normalize(cross(eyeNormal, eyeTangent));
	texCoord0 = gl_MultiTexCoord0;
	texCoord1 = gl_MultiTexCoord1;
	gl_Position = ftransform();
	gl_FrontColor = gl_Color;
	gl_BackColor = gl_Color;
}
