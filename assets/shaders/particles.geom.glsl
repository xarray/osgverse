uniform vec4 DataRange;
VERSE_VS_IN vec4 color_gs[], texCoord_gs[];
VERSE_VS_IN float animationID_gs[], lifeTime_gs[];
VERSE_VS_OUT vec4 color, texCoord;
VERSE_VS_OUT float animationID, lifeTime;

void main()
{
    vec2 e[4] = vec2[](vec2(-0.05, -0.05), vec2(0.05, -0.05), vec2(0.05, 0.05), vec2(-0.05, 0.05));
    vec2 uv[4] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));
    int index[6] = int[](0, 1, 2, 0, 2, 3);

    for (int i = 0; i < 6; ++i)
    {
        int id = index[i];
        animationID = animationID_gs[0]; lifeTime = lifeTime_gs[0];
        color = color_gs[0]; texCoord = vec4(uv[id], 0.0, 0.0);
        if (DataRange.z > 2.5)  // ray
        {
            vec4 viewPos = vec4(e[id], 0.0, 0.0) + gl_in[0].gl_Position;
            if (id > 1) viewPos += vec4(texCoord_gs[0].yzw, 0.0);
            gl_Position = (VERSE_MATRIX_MVP * viewPos) + vec4(e[id] * texCoord_gs[0].x, 0.0, 0.0);
        }
        else if (DataRange.z > 1.5)  // billboard (no-scale)
        {
            vec4 viewPos = gl_in[0].gl_Position + vec4(e[id] * texCoord_gs[0].x, 0.0, 0.0);
            gl_Position = VERSE_MATRIX_P * viewPos;
        }
        else if (DataRange.z > 0.5)  // billboard (with-scale)
        {
            vec4 viewPos = VERSE_MATRIX_MVP * (vec4(e[id], 0.0, 0.0) + gl_in[0].gl_Position);
            gl_Position = viewPos + vec4(e[id] * texCoord_gs[0].x, 0.0, 0.0);
        }
        else  // etc.
            gl_Position = VERSE_MATRIX_MVP * (vec4(e[id], 0.0, 0.0) + gl_in[0].gl_Position);
        EmitVertex();
        if (i == 2 || i == 5) EndPrimitive();
    }
}
