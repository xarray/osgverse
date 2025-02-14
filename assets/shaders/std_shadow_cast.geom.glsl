#extension GL_EXT_geometry_shader4 : enable
VERSE_VS_IN vec4 texCoord0_gs[], lightProjVec_gs[];
VERSE_VS_OUT vec4 texCoord0, lightProjVec;
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
            vec4 tmp = TSP * (Tv + gl_PositionIn[i]);
            lightProjVec = tmp;
            lightProjVec.xy += (vec2(float(Sx), float(Sy)) / NV) * tmp.w * 2.0;
            texCoord0 = texCoord0_gs[i]; gl_Position = lightProjVec;
            EmitVertex();
        }
        EndPrimitive();
        Tv.x = eyeSep[k];
    }
}
