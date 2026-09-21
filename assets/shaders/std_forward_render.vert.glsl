#pragma import_defines(VERSE_VRMODE)
VERSE_VS_IN vec4 osg_Tangent;
#ifdef VERSE_VRMODE
VERSE_VS_OUT vec4 texCoord0_gs, texCoord1_gs, color_gs, eyeVertex_gs;
VERSE_VS_OUT vec3 eyeNormal_gs, eyeTangent_gs, eyeBinormal_gs;
#else
VERSE_VS_OUT vec4 texCoord0, texCoord1, color, eyeVertex;
VERSE_VS_OUT vec3 eyeNormal, eyeTangent, eyeBinormal;
#endif
uniform mat4 osg_ViewMatrixInverse;

// A model is not required to provide a tangent: its attribute then reads back as zero, and
// normalize() would turn it into NaN, which spreads into the lighting and blackens the whole
// object. Such a zero is kept as zero by the inversesqrt() guard below, and the fragment shader
// detects it (see std_forward_render.frag.glsl) to give up normal mapping instead of using a
// broken TBN matrix
void createEyeTangentFrame(in vec3 objectNormal, in vec4 objectTangent,
                           out vec3 eyeTangent, out vec3 eyeBinormal)
{
    vec3 t = VERSE_MATRIX_N * objectTangent.xyz;
    vec3 b = VERSE_MATRIX_N * (cross(objectNormal, objectTangent.xyz) * objectTangent.w);
    eyeTangent = t * inversesqrt(max(dot(t, t), 1e-12));
    eyeBinormal = b * inversesqrt(max(dot(b, b), 1e-12));
}

void main()
{
#ifdef VERSE_VRMODE
    eyeNormal_gs = normalize(VERSE_MATRIX_N * osg_Normal);
    createEyeTangentFrame(osg_Normal, osg_Tangent, eyeTangent_gs, eyeBinormal_gs);
    eyeVertex_gs = osg_ViewMatrixInverse * VERSE_MATRIX_MV * osg_Vertex;

    texCoord0_gs = osg_MultiTexCoord0;
    texCoord1_gs = osg_MultiTexCoord1;
    color_gs = osg_Color;
    gl_Position = eyeVertex_gs;
#else
    eyeNormal = normalize(VERSE_MATRIX_N * osg_Normal);
    createEyeTangentFrame(osg_Normal, osg_Tangent, eyeTangent, eyeBinormal);
    eyeVertex = VERSE_MATRIX_MV * osg_Vertex;

    texCoord0 = osg_MultiTexCoord0;
    texCoord1 = osg_MultiTexCoord1;
    color = osg_Color;
    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;
#endif
}
