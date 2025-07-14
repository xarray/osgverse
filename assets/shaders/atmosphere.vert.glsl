uniform mat4 osg_ViewMatrixInverse;
uniform mat4 osg_ProjectionMatrixInverse;
VERSE_VS_OUT vec3 eyeSpaceRay;

void main()
{
    vec3 eyesSpacePos = (osg_ProjectionMatrixInverse * osg_Vertex).xyz;
    eyeSpaceRay = (osg_ViewMatrixInverse * vec4(eyesSpacePos, 0.0)).xyz;
    gl_Position = osg_Vertex;
})
