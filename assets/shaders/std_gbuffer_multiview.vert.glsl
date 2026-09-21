VERSE_VS_IN vec3 osg_Tangent, osg_Binormal;
VERSE_VS_OUT vec4 texCoord0_gs, texCoord1_gs, color_gs;
VERSE_VS_OUT vec3 eyeNormal_gs, eyeTangent_gs, eyeBinormal_gs;

// A model is not required to provide a tangent and a binormal: these attributes then read back as
// zero, and normalize() would turn them into NaN, which spreads into the lighting and blackens the
// whole object. Such zeros are kept as zeros by the inversesqrt() guard below, and the fragment
// shader detects them (see std_gbuffer.frag.glsl) to give up normal mapping instead of using a
// broken TBN matrix
void createEyeTangentFrame(in vec3 objectTangent, in vec3 objectBinormal,
                           out vec3 eyeTangent, out vec3 eyeBinormal)
{
    vec3 t = VERSE_MATRIX_N * objectTangent;
    vec3 b = VERSE_MATRIX_N * objectBinormal;
    eyeTangent = t * inversesqrt(max(dot(t, t), 1e-12));
    eyeBinormal = b * inversesqrt(max(dot(b, b), 1e-12));
}

void main()
{
    eyeNormal_gs = normalize(VERSE_MATRIX_N * osg_Normal);
    createEyeTangentFrame(osg_Tangent, osg_Binormal, eyeTangent_gs, eyeBinormal_gs);

    texCoord0_gs = osg_MultiTexCoord0;
    texCoord1_gs = osg_MultiTexCoord1;
    color_gs = osg_Color;
    gl_Position = VERSE_MATRIX_MV * osg_Vertex;
}
