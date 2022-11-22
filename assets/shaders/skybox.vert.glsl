uniform mat4 osg_ViewMatrixInverse;
uniform mat4 SkyTextureMatrix;
VERSE_VS_OUT vec3 texCoord;

void main()
{
    vec3 N = normalize(VERSE_MATRIX_N * osg_Normal);
    vec3 eyeDir = normalize(VERSE_MATRIX_MV * osg_Vertex).xyz;
    vec4 uvData = (osg_ViewMatrixInverse * vec4(reflect(-eyeDir, N), 0.0));
    texCoord = (SkyTextureMatrix * uvData).xyz;
    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;
}