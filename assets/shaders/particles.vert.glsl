#extension GL_EXT_draw_instanced : enable
uniform sampler2D PosColorTexture, VelocityTexture;
uniform vec3 DataRange;
VERSE_VS_OUT vec4 color, texCoord;
const float RES = 2048.0;

void main()
{
    float r = float(gl_InstanceID - int(DataRange.x)) / RES;
    float c = floor(r) / RES; r = fract(r);
    vec4 posSize = VERSE_TEX2D(PosColorTexture, vec2(r, c));
    color = VERSE_TEX2D(PosColorTexture, vec2(r, c + 0.5));
    texCoord = osg_MultiTexCoord0;

    if (DataRange.z > 0.5f)
    {
        vec4 viewPos = VERSE_MATRIX_MV * vec4(posSize.xyz, 1.0);
        viewPos = viewPos + vec4(osg_Vertex.xy * (-viewPos.z), 0.0, 0.0);
        gl_Position = VERSE_MATRIX_P * viewPos;
    }
    else
        gl_Position = VERSE_MATRIX_MVP * (osg_Vertex + vec4(posSize.xyz, 0.0));
}
