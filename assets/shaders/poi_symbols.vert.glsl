#pragma import_defines(FAR_DISTANCE, MID_DISTANCE)
#extension GL_EXT_draw_instanced : enable
uniform sampler2D PosTexture, DirTexture, ColorTexture;
uniform vec3 Offset, Scale;
uniform float InvResolution;
VERSE_VS_OUT vec4 UserColor;
VERSE_VS_OUT vec2 TexCoord, TexCoordBG;

mat4 rotationMatrix(vec3 axis0, float angle)
{
    float s = sin(angle), c = cos(angle);
    float oc = 1.0 - c; vec3 a = normalize(axis0);
    return mat4(oc * a.x * a.x + c, oc * a.x * a.y - a.z * s, oc * a.z * a.x + a.y * s, 0.0,
        oc * a.x * a.y + a.z * s, oc * a.y * a.y + c, oc * a.y * a.z - a.x * s, 0.0,
        oc * a.z * a.x - a.y * s, oc * a.y * a.z + a.x * s, oc * a.z * a.z + c, 0.0,
        0.0, 0.0, 0.0, 1.0);
}

void main()
{
    float r = float(gl_InstanceID) * InvResolution;
    float c = floor(r) * InvResolution; r = fract(r);
    vec4 pos = texture2D(PosTexture, vec2(r, c));
    vec4 dir = texture2D(DirTexture, vec2(r, c));
    UserColor = texture2D(ColorTexture, vec2(r, c));
#ifdef MID_DISTANCE
    float tx = float(gl_InstanceID) * Scale.z;
    float ty = floor(tx) * Scale.z; tx = fract(tx);
    TexCoord = vec2(tx, ty) + osg_MultiTexCoord0.xy * Scale.z;
#endif
    TexCoordBG = osg_MultiTexCoord0.xy * dir.z + dir.xy;

    mat4 proj = VERSE_MATRIX_P;
    float ar = proj[0][0] / proj[1][1];
#ifdef FAR_DISTANCE
    vec4 v0 = vec4(osg_Vertex.xyz * pos.w, 1.0), projP = proj * vec4(pos.xyz, 1.0);
    vec4 v1 = rotationMatrix(vec3(0.0, 0.0, 1.0), dir.w) * v0;
    gl_Position = vec4(vec3(v1.x * ar, v1.yz) / v1.w + projP.xyz / projP.w, 1.0);
#else
    vec4 v0 = vec4((osg_Vertex.xyz + Offset) * pos.w, 1.0);
    vec4 v1 = proj * vec4(pos.xyz, 1.0); v1 = v1 / v1.w;
    gl_Position = vec4(vec3(v0.x * Scale.x * ar, v0.y * Scale.y, v0.z) / v0.w + v1.xyz, 1.0);
#endif   
}
