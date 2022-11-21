uniform mat4 osg_ViewMatrixInverse;
uniform mat4 SkyTextureMatrix;
VERSE_VS_OUT vec3 texCoord;

void main()
{
    gl_Position = VERSE_MATRIX_MVP * gl_Vertex;
    vec3 N = normalize(VERSE_MATRIX_N * gl_Normal);
    vec3 eyeDir = normalize(VERSE_MATRIX_MV * gl_Vertex).xyz;
    vec4 uvData = (osg_ViewMatrixInverse * vec4(reflect(-eyeDir, N), 0.0));
    texCoord = (SkyTextureMatrix * uvData).xyz;
}