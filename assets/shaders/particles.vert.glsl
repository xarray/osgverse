#extension GL_EXT_draw_instanced : enable
#ifdef USE_VERTEX_ATTRIB
VERSE_VS_IN vec4 osg_UserPosition, osg_UserColor, osg_UserVelocity, osg_UserEulers;
#else
uniform sampler2D PosColorTexture, VelocityTexture;
#endif
uniform vec4 DataRange;
VERSE_VS_OUT vec4 color, texCoord;
VERSE_VS_OUT float animationID, lifeTime;
const float RES = 2048.0, SCALE_FACTOR = 100.0;

void main()
{
#ifdef USE_VERTEX_ATTRIB
    vec4 posSize = osg_UserPosition;
    vec4 velocityLife = osg_UserVelocity;
    vec4 eulerAnim = osg_UserEulers;
    color = osg_UserColor;
#else
    float r = float(gl_InstanceID + int(DataRange.x)) / RES;
    float c = floor(r) / RES; r = fract(r);
    vec4 posSize = VERSE_TEX2D(PosColorTexture, vec2(r, c));
    vec4 velocityLife = VERSE_TEX2D(VelocityTexture, vec2(r, c));
    vec4 eulerAnim = VERSE_TEX2D(VelocityTexture, vec2(r, c + 0.5));
    color = VERSE_TEX2D(PosColorTexture, vec2(r, c + 0.5));
#endif
    animationID = eulerAnim.a; lifeTime = velocityLife.a;

    float size = posSize.w; texCoord = osg_MultiTexCoord0;
    if (DataRange.z > 1.5)  // billboard (no-scale)
    {
        vec4 viewPos = VERSE_MATRIX_MV * vec4(posSize.xyz, 1.0);
        viewPos = viewPos + vec4(osg_Vertex.xy * (-viewPos.z * size), 0.0, 0.0);
        gl_Position = VERSE_MATRIX_P * viewPos;
    }
    else if (DataRange.z > 0.5)  // billboard (with-scale)
    {
        vec2 scale = vec2(length(VERSE_MATRIX_MV[0]) / DataRange.a, length(VERSE_MATRIX_MV[1]));
        vec4 viewPos = VERSE_MATRIX_MVP * (osg_Vertex + vec4(posSize.xyz, 0.0));
        gl_Position = viewPos + vec4(osg_Vertex.xy * (scale * size * SCALE_FACTOR), 0.0, 0.0);
    }
    else  // mesh etc.
        gl_Position = VERSE_MATRIX_MVP * (osg_Vertex + vec4(posSize.xyz, 0.0));
}
