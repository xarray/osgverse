uniform mat4 osg_ViewMatrixInverse;
VERSE_VS_OUT vec3 normalInWorld;
VERSE_VS_OUT vec3 vertexInWorld;
VERSE_VS_OUT vec4 texCoord;

void main()
{
    normalInWorld = normalize(vec3(osg_ViewMatrixInverse * vec4(VERSE_MATRIX_N * gl_Normal, 0.0)));
    vertexInWorld = vec3(osg_ViewMatrixInverse * VERSE_MATRIX_MV * osg_Vertex);
    texCoord = osg_MultiTexCoord0;
    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;;
}
