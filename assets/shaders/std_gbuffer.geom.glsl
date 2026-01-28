#extension GL_EXT_geometry_shader4 : enable
VERSE_VS_IN vec4 texCoord0_gs[], texCoord1_gs[], color_gs[];
VERSE_VS_IN vec3 eyeNormal_gs[], eyeTangent_gs[], eyeBinormal_gs[];
VERSE_VS_OUT vec4 texCoord0, texCoord1, color;
VERSE_VS_OUT vec3 eyeNormal, eyeTangent, eyeBinormal;
uniform float eyeSep[2];

void main()
{
    vec2 NV = vec2(2, 1); int numViews = 2;
    vec2 T = -1.0 + 1.0 / NV;
    mat4x4 TSP = mat4x4(1.0 / NV.x, 0.0, 0.0, 0.0,
                        0.0, 1.0 / NV.y, 0.0, 0.0,
                        0.0, 0.0, 1.0, 0.0,
                        T.x, T.y, 0.0, 1.0) * VERSE_MATRIX_P;
    vec4 Tv = vec4(eyeSep[0], 0.0, 0.0, 0.0);
    for (int k = 0; k < numViews; ++k)
    {
        int Sx = k % int(NV.x), Sy = int(floor(k / NV.x));
        for (int i = 0; i < 3; ++i)
        {
            vec4 tmp = TSP * (Tv + VERSE_GS_POS(i));
            vec2 coeff = (vec2(2.0) / NV) * tmp.w;
            gl_ClipDistance[0] = tmp.x + tmp.w;
            gl_ClipDistance[1] = coeff.x - (tmp.x + tmp.w);
            gl_ClipDistance[2] = tmp.y + tmp.w;
            gl_ClipDistance[3] = coeff.y - (tmp.y + tmp.w);

            eyeNormal = eyeNormal_gs[i]; eyeTangent = eyeTangent_gs[i];
            eyeBinormal = eyeBinormal_gs[i]; color = color_gs[i];
            texCoord0 = texCoord0_gs[i]; texCoord1 = texCoord1_gs[i];
            gl_Position = tmp;
            gl_Position.xy += (vec2(float(Sx), float(Sy)) / NV) * tmp.w * 2.0;
            EmitVertex();
        }
        EndPrimitive();
        Tv.x = eyeSep[k];
    }
}
