#extension GL_EXT_draw_instanced : enable
#pragma import_defines(USE_INSTANCING, USE_INSTANCING_TEXARRAY, FULL_SH)

uniform mat4 osg_ViewMatrixInverse;
uniform vec2 NearFarPlanes, InvScreenResolution;
uniform float GaussianRenderingMode;
#if defined(USE_INSTANCING)
#  if defined(USE_INSTANCING_TEXARRAY)
uniform sampler2DArray CoreParameters, ShParameters;
uniform vec2 TextureSize;
#  else
layout(std140, binding = 0) restrict readonly buffer CorePosBuffer { vec4 corePos[]; };
layout(std140, binding = 1) restrict readonly buffer CoreCov0Buffer { vec4 coreCov0[]; };
layout(std140, binding = 2) restrict readonly buffer CoreCov1Buffer { vec4 coreCov1[]; };
layout(std140, binding = 3) restrict readonly buffer CoreCov2Buffer { vec4 coreCov2[]; };

struct ShcoefData
{
    vec4 rgb0; vec4 rgb1; vec4 rgb2; vec4 rgb3; vec4 rgb4;
    vec4 rgb5; vec4 rgb6; vec4 rgb7; vec4 rgb8; vec4 rgb9;
    vec4 rgb10; vec4 rgb11; vec4 rgb12; vec4 rgb13; vec4 rgb14;
};
layout(std140, binding = 4) restrict readonly buffer ShcoefBuffer { ShcoefData shcoef[]; };
#  endif

VERSE_VS_IN float osg_UserIndex;
VERSE_VS_OUT vec4 color, invCovariance;
VERSE_VS_OUT vec2 center2D;

mat2 inverseMat2(mat2 m)
{
    float det = m[0][0] * m[1][1] - m[0][1] * m[1][0]; mat2 inv;
    inv[0][0] = m[1][1] / det; inv[0][1] = -m[0][1] / det;
    inv[1][0] = -m[1][0] / det; inv[1][1] = m[0][0] / det;
    return inv;
}
#else
VERSE_VS_IN vec4 osg_Covariance0, osg_Covariance1, osg_Covariance2;
VERSE_VS_IN vec4 osg_R_SH0, osg_G_SH0, osg_B_SH0;
VERSE_VS_IN vec4 osg_R_SH1, osg_G_SH1, osg_B_SH1;
VERSE_VS_IN vec4 osg_R_SH2, osg_G_SH2, osg_B_SH2;
VERSE_VS_IN vec4 osg_R_SH3, osg_G_SH3, osg_B_SH3;

VERSE_VS_OUT vec4 color_gs, covariance_gs;
VERSE_VS_OUT vec2 center2D_gs;
#endif

vec3 computeRadianceFromSH(in vec3 v, in vec3 baseColor, in vec2 paramUV)
{
#ifdef FULL_SH
    float b[16];
#else
    float b[4];
#endif
    float vx2 = v.x * v.x;
    float vy2 = v.y * v.y;
    float vz2 = v.z * v.z;

    float k1 = 0.4886025119029199f;  // first order (/ (sqrt 3.0) (* 2 (sqrt pi)))
    b[0] = 0.28209479177387814f;     // zeroth order (/ 1.0 (* 2.0 (sqrt pi)))
    b[1] = -k1 * v.y; b[2] = k1 * v.z; b[3] = -k1 * v.x;

#ifdef FULL_SH
    // second order
    float k2 = 1.0925484305920792f;   // (/ (sqrt 15.0) (* 2 (sqrt pi)))
    float k3 = 0.31539156525252005f;  // (/ (sqrt 5.0) (* 4 (sqrt  pi)))
    float k4 = 0.5462742152960396f;   // (/ (sqrt 15.0) (* 4 (sqrt pi)))
    b[4] = k2 * v.y * v.x;
    b[5] = -k2 * v.y * v.z;
    b[6] = k3 * (3.0f * vz2 - 1.0f);
    b[7] = -k2 * v.x * v.z;
    b[8] = k4 * (vx2 - vy2);

    // third order
    float k5 = 0.5900435899266435f;  // (/ (* (sqrt 2) (sqrt 35)) (* 8 (sqrt pi)))
    float k6 = 2.8906114426405543f;  // (/ (sqrt 105) (* 2 (sqrt pi)))
    float k7 = 0.4570457994644658f;  // (/ (* (sqrt 2) (sqrt 21)) (* 8 (sqrt pi)))
    float k8 = 0.37317633259011546f; // (/ (sqrt 7) (* 4 (sqrt pi)))
    float k9 = 1.4453057213202771f;  // (/ (sqrt 105) (* 4 (sqrt pi)))
    b[9] = -k5 * v.y * (3.0f * vx2 - vy2);
    b[10] = k6 * v.y * v.x * v.z;
    b[11] = -k7 * v.y * (5.0f * vz2 - 1.0f);
    b[12] = k8 * v.z * (5.0f * vz2 - 3.0f);
    b[13] = -k7 * v.x * (5.0f * vz2 - 1.0f);
    b[14] = k9 * v.z * (vx2 - vy2);
    b[15] = -k5 * v.x * (vx2 - 3.0f * vy2);

#  if defined(USE_INSTANCING)
#    if defined(USE_INSTANCING_TEXARRAY)
    vec4 sh_rgb0 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 0.0));
    vec4 sh_rgb1 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 1.0));
    vec4 sh_rgb2 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 2.0));
    vec4 sh_rgb3 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 3.0));
    vec4 sh_rgb4 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 4.0));
    vec4 sh_rgb5 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 5.0));
    vec4 sh_rgb6 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 6.0));
    vec4 sh_rgb7 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 7.0));
    vec4 sh_rgb8 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 8.0));
    vec4 sh_rgb9 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 9.0));
    vec4 sh_rgb10 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 10.0));
    vec4 sh_rgb11 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 11.0));
    vec4 sh_rgb12 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 12.0));
    vec4 sh_rgb13 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 13.0));
    vec4 sh_rgb14 = VERSE_TEX2DARRAY(ShParameters, vec3(paramUV, 14.0));
#    else
    ShcoefData shData = shcoef[uint(osg_UserIndex)];
    vec4 sh_rgb0 = shData.rgb0, sh_rgb1 = shData.rgb1, sh_rgb2 = shData.rgb2, sh_rgb3 = shData.rgb3, sh_rgb4 = shData.rgb4;
    vec4 sh_rgb5 = shData.rgb5, sh_rgb6 = shData.rgb6, sh_rgb7 = shData.rgb7, sh_rgb8 = shData.rgb8, sh_rgb9 = shData.rgb9;
    vec4 sh_rgb10 = shData.rgb10, sh_rgb11 = shData.rgb11, sh_rgb12 = shData.rgb12, sh_rgb13 = shData.rgb13, sh_rgb14 = shData.rgb14;
#    endif
    float re = (b[0] * baseColor.x + b[1] * sh_rgb0.x + b[2] * sh_rgb1.x + b[3] * sh_rgb2.x +
                b[4] * sh_rgb3.x + b[5] * sh_rgb4.x + b[6] * sh_rgb5.x + b[7] * sh_rgb6.x +
                b[8] * sh_rgb7.x + b[9] * sh_rgb8.x + b[10] * sh_rgb9.x + b[11] * sh_rgb10.x +
                b[12] * sh_rgb11.x + b[13] * sh_rgb12.x + b[14] * sh_rgb13.x + b[15] * sh_rgb14.x);
    float gr = (b[0] * baseColor.y + b[1] * sh_rgb0.y + b[2] * sh_rgb1.y + b[3] * sh_rgb2.y +
                b[4] * sh_rgb3.y + b[5] * sh_rgb4.y + b[6] * sh_rgb5.y + b[7] * sh_rgb6.y +
                b[8] * sh_rgb7.y + b[9] * sh_rgb8.y + b[10] * sh_rgb9.y + b[11] * sh_rgb10.y +
                b[12] * sh_rgb11.y + b[13] * sh_rgb12.y + b[14] * sh_rgb13.y + b[15] * sh_rgb14.y);
    float bl = (b[0] * baseColor.z + b[1] * sh_rgb0.z + b[2] * sh_rgb1.z + b[3] * sh_rgb2.z +
                b[4] * sh_rgb3.z + b[5] * sh_rgb4.z + b[6] * sh_rgb5.z + b[7] * sh_rgb6.z +
                b[8] * sh_rgb7.z + b[9] * sh_rgb8.z + b[10] * sh_rgb9.z + b[11] * sh_rgb10.z +
                b[12] * sh_rgb11.z + b[13] * sh_rgb12.z + b[14] * sh_rgb13.z + b[15] * sh_rgb14.z);
#  else
    float re = (b[0] * baseColor.x + b[1] * osg_R_SH0.y + b[2] * osg_R_SH0.z + b[3] * osg_R_SH0.w +
                b[4] * osg_R_SH1.x + b[5] * osg_R_SH1.y + b[6] * osg_R_SH1.z + b[7] * osg_R_SH1.w +
                b[8] * osg_R_SH2.x + b[9] * osg_R_SH2.y + b[10] * osg_R_SH2.z + b[11] * osg_R_SH2.w +
                b[12] * osg_R_SH3.x + b[13] * osg_R_SH3.y + b[14] * osg_R_SH3.z + b[15] * osg_R_SH3.w);
    float gr = (b[0] * baseColor.y + b[1] * osg_G_SH0.y + b[2] * osg_G_SH0.z + b[3] * osg_G_SH0.w +
                b[4] * osg_G_SH1.x + b[5] * osg_G_SH1.y + b[6] * osg_G_SH1.z + b[7] * osg_G_SH1.w +
                b[8] * osg_G_SH2.x + b[9] * osg_G_SH2.y + b[10] * osg_G_SH2.z + b[11] * osg_G_SH2.w +
                b[12] * osg_G_SH3.x + b[13] * osg_G_SH3.y + b[14] * osg_G_SH3.z + b[15] * osg_G_SH3.w);
    float bl = (b[0] * baseColor.z + b[1] * osg_B_SH0.y + b[2] * osg_B_SH0.z + b[3] * osg_B_SH0.w +
                b[4] * osg_B_SH1.x + b[5] * osg_B_SH1.y + b[6] * osg_B_SH1.z + b[7] * osg_B_SH1.w +
                b[8] * osg_B_SH2.x + b[9] * osg_B_SH2.y + b[10] * osg_B_SH2.z + b[11] * osg_B_SH2.w +
                b[12] * osg_B_SH3.x + b[13] * osg_B_SH3.y + b[14] * osg_B_SH3.z + b[15] * osg_B_SH3.w);
#  endif
#else
    float re = b[0] * baseColor.x, gr = b[0] * baseColor.y, bl = b[0] * baseColor.z;
#endif
    return vec3(0.5f, 0.5f, 0.5f) + vec3(re, gr, bl);
}

void main()
{
    vec2 paramUV = vec2(0.0, 0.0);
#if defined(USE_INSTANCING)
#  if defined(USE_INSTANCING_TEXARRAY)
    float r = float(osg_UserIndex) / TextureSize.x;
    float c = floor(r) / TextureSize.y; paramUV = vec2(fract(r), c);
    vec4 posAlpha = VERSE_TEX2DARRAY(CoreParameters, vec3(paramUV, 0.0));
    vec4 cov0 = VERSE_TEX2DARRAY(CoreParameters, vec3(paramUV, 1.0));
    vec4 cov1 = VERSE_TEX2DARRAY(CoreParameters, vec3(paramUV, 2.0));
    vec4 cov2 = VERSE_TEX2DARRAY(CoreParameters, vec3(paramUV, 3.0));
#  else
    uint index = uint(osg_UserIndex);
    vec4 posAlpha = corePos[index];
    vec4 cov0 = coreCov0[index], cov1 = coreCov1[index], cov2 = coreCov2[index];
#  endif
    vec4 eyeVertex = VERSE_MATRIX_MV * vec4(posAlpha.xyz, 1.0);
    float alpha = posAlpha.w, FAR_NEAR = NearFarPlanes.y - NearFarPlanes.x;
#else
    vec4 eyeVertex = VERSE_MATRIX_MV * vec4(osg_Vertex.xyz, 1.0);
    float alpha = osg_Covariance0.w, FAR_NEAR = NearFarPlanes.y - NearFarPlanes.x;
#endif
    float WIDTH = 1.0 / InvScreenResolution.x, HEIGHT = 1.0 / InvScreenResolution.y;

    // J is the jacobian of the projection and viewport transformations.
    // this is an affine approximation of the real projection.
    float SX = VERSE_MATRIX_P[0][0], SY = VERSE_MATRIX_P[1][1];
    float WZ = VERSE_MATRIX_P[3][2], eyeZsq = eyeVertex.z * eyeVertex.z;
    float jsx = -(SX * WIDTH) / (2.0f * eyeVertex.z);
    float jsy = -(SY * HEIGHT) / (2.0f * eyeVertex.z);
    float jtx = (SX * eyeVertex.x * WIDTH) / (2.0f * eyeZsq);
    float jty = (SY * eyeVertex.y * HEIGHT) / (2.0f * eyeZsq);
    float jtz = (FAR_NEAR * WZ) / (2.0f * eyeZsq);
    mat3 J = mat3(vec3(jsx, 0.0f, 0.0f), vec3(0.0f, jsy, 0.0f), vec3(jtx, jty, jtz));

    // combine the affine transforms of W (viewMat) and J (approx of viewportMat * projMat)
    // using the fact that the new transformed covariance matrix V_Prime = JW * V * (JW)^T
#if defined(USE_INSTANCING)
    mat3 V = mat3(cov0.xyz, cov1.xyz, cov2.xyz);
#else
    mat3 V = mat3(osg_Covariance0.xyz, osg_Covariance1.xyz, osg_Covariance2.xyz);
#endif
    if (GaussianRenderingMode > 0.5) V = mat3(0.001, 0.0, 0.0, 0.0, 0.001, 0.0, 0.0, 0.0, 0.001);

    mat3 W = mat3(VERSE_MATRIX_MV); mat3 JW = J * W; mat3 V_prime = JW * V * transpose(JW);
    mat2 cov2D = mat2(V_prime);  // 'project' the 3D covariance matrix onto xy plane
    float X0 = 0.0, Y0 = 0.0;  // viewport X & Y... FIXME: always 0?
    vec4 proj = VERSE_MATRIX_P * eyeVertex;
    cov2D[0][0] += 0.3f; cov2D[1][1] += 0.3f;  // The convolution of a gaussian with another is the sum of their
                                               // covariance matrices, apply a low-pass filter for antialiasing
    
#if defined(USE_INSTANCING)
    vec4 covariance = vec4(cov2D[0], cov2D[1]);
    center2D = vec2(proj.x / proj.w, proj.y / proj.w);
    center2D.x = 0.5f * (WIDTH + (center2D.x * WIDTH) + (2.0f * X0));
    center2D.y = 0.5f * (HEIGHT + (center2D.y * HEIGHT) + (2.0f * Y0));
#else
    covariance_gs = vec4(cov2D[0], cov2D[1]);
    center2D_gs = vec2(proj.x / proj.w, proj.y / proj.w);
    center2D_gs.x = 0.5f * (WIDTH + (center2D_gs.x * WIDTH) + (2.0f * X0));
    center2D_gs.y = 0.5f * (HEIGHT + (center2D_gs.y * HEIGHT) + (2.0f * Y0));
#endif

    // compute radiance from SH
    vec3 eyeDirection = normalize(eyeVertex.xyz / eyeVertex.w);
    //vec3 direction = transpose(mat3(osg_ViewMatrixInverse)) * eyeDirection;
#if defined(USE_INSTANCING)
    vec3 baseColor = vec3(cov0.w, cov1.w, cov2.w);
    color = vec4(computeRadianceFromSH(eyeDirection, baseColor, paramUV), alpha);

    vec3 ndcP = proj.xyz / proj.w;
    if (!(ndcP.z < 0.25 || ndcP.x > 2.0 || ndcP.x < -2.0 || ndcP.y > 2.0 || ndcP.y < -2.0))
    {
        mat2 cov2Dinv = inverseMat2(cov2D);
        vec4 cov2Dinv4 = vec4(cov2Dinv[0], cov2Dinv[1]);

        // compute 2d extents for the splat, using covariance matrix ellipse (https://cookierobotics.com/007/)
        float k = 3.5, a = cov2D[0][0], b = cov2D[0][1], c = cov2D[1][1];
        float apco2 = (a + c) / 2.0, amco2 = (a - c) / 2.0;
        float term = sqrt(amco2 * amco2 + b * b);
        float maj = apco2 + term, min = apco2 - term;

        float theta = (b == 0.0) ? ((a >= c) ? 0.0 : radians(90.0)) : atan(maj - a, b);
        float r1 = k * sqrt(maj), r2 = k * sqrt(min);
        vec2 majAxis = vec2(r1 * cos(theta), r1 * sin(theta));
        vec2 minAxis = vec2(r2 * cos(theta + radians(90.0)), r2 * sin(theta + radians(90.0)));

        vec2 offset = majAxis * osg_Vertex.x + minAxis * osg_Vertex.y;
        offset.x *= (2.0 * InvScreenResolution.x) * proj.w;
        offset.y *= (2.0 * InvScreenResolution.y) * proj.w;
        invCovariance = cov2Dinv4; proj.xy += offset;
    }
#else
    vec3 baseColor = vec3(osg_R_SH0.x, osg_G_SH0.x, osg_B_SH0.x);
    color_gs = vec4(computeRadianceFromSH(eyeDirection, baseColor, paramUV), alpha);
#endif
    gl_Position = proj;
}
