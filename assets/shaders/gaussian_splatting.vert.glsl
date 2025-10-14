uniform mat4 osg_ViewMatrixInverse;
uniform vec2 NearFarPlanes, InvScreenResolution;

VERSE_VS_IN vec4 osg_Covariance0, osg_Covariance1, osg_Covariance2;
VERSE_VS_IN vec4 osg_R_SH0, osg_G_SH0, osg_B_SH0;
VERSE_VS_IN vec4 osg_R_SH1, osg_G_SH1, osg_B_SH1;
VERSE_VS_IN vec4 osg_R_SH2, osg_G_SH2, osg_B_SH2;
VERSE_VS_IN vec4 osg_R_SH3, osg_G_SH3, osg_B_SH3;

VERSE_VS_OUT vec4 color_gs, covariance_gs;
VERSE_VS_OUT vec2 center2D_gs;

#define FULL_SH 1
vec3 computeRadianceFromSH(const vec3 v)
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

    float re = (b[0] * osg_R_SH0.x + b[1] * osg_R_SH0.y + b[2] * osg_R_SH0.z + b[3] * osg_R_SH0.w +
                b[4] * osg_R_SH1.x + b[5] * osg_R_SH1.y + b[6] * osg_R_SH1.z + b[7] * osg_R_SH1.w +
                b[8] * osg_R_SH2.x + b[9] * osg_R_SH2.y + b[10] * osg_R_SH2.z + b[11] * osg_R_SH2.w +
                b[12] * osg_R_SH3.x + b[13] * osg_R_SH3.y + b[14] * osg_R_SH3.z + b[15] * osg_R_SH3.w);
    float gr = (b[0] * osg_G_SH0.x + b[1] * osg_G_SH0.y + b[2] * osg_G_SH0.z + b[3] * osg_G_SH0.w +
                b[4] * osg_G_SH1.x + b[5] * osg_G_SH1.y + b[6] * osg_G_SH1.z + b[7] * osg_G_SH1.w +
                b[8] * osg_G_SH2.x + b[9] * osg_G_SH2.y + b[10] * osg_G_SH2.z + b[11] * osg_G_SH2.w +
                b[12] * osg_G_SH3.x + b[13] * osg_G_SH3.y + b[14] * osg_G_SH3.z + b[15] * osg_G_SH3.w);
    float bl = (b[0] * osg_B_SH0.x + b[1] * osg_B_SH0.y + b[2] * osg_B_SH0.z + b[3] * osg_B_SH0.w +
                b[4] * osg_B_SH1.x + b[5] * osg_B_SH1.y + b[6] * osg_B_SH1.z + b[7] * osg_B_SH1.w +
                b[8] * osg_B_SH2.x + b[9] * osg_B_SH2.y + b[10] * osg_B_SH2.z + b[11] * osg_B_SH2.w +
                b[12] * osg_B_SH3.x + b[13] * osg_B_SH3.y + b[14] * osg_B_SH3.z + b[15] * osg_B_SH3.w);
#else
    float re = (b[0] * osg_R_SH0.x + b[1] * osg_R_SH0.y + b[2] * osg_R_SH0.z + b[3] * osg_R_SH0.w);
    float gr = (b[0] * osg_G_SH0.x + b[1] * osg_G_SH0.y + b[2] * osg_G_SH0.z + b[3] * osg_G_SH0.w);
    float bl = (b[0] * osg_B_SH0.x + b[1] * osg_B_SH0.y + b[2] * osg_B_SH0.z + b[3] * osg_B_SH0.w);
#endif
    return vec3(0.5f, 0.5f, 0.5f) + vec3(re, gr, bl);
}

void main()
{
    vec4 eyeVertex = VERSE_MATRIX_MV * vec4(osg_Vertex.xyz, 1.0);
    float alpha = osg_Covariance0.w, FAR_NEAR = NearFarPlanes.y - NearFarPlanes.x;
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
    mat3 W = mat3(VERSE_MATRIX_MV), V = mat3(osg_Covariance0.xyz, osg_Covariance1.xyz, osg_Covariance2.xyz);
    mat3 JW = J * W; mat3 V_prime = JW * V * transpose(JW);
    mat2 cov2D = mat2(V_prime);  // 'project' the 3D covariance matrix onto xy plane

    // use the fact that the convolution of a gaussian with another gaussian is the sum
    // of their covariance matrices to apply a low-pass filter to anti-alias the splats
    cov2D[0][0] += 0.3f; cov2D[1][1] += 0.3f;
    covariance_gs = vec4(cov2D[0], cov2D[1]);

    float X0 = 0.0, Y0 = 0.0;  // viewport X & Y... FIXME: always 0?
    vec4 proj = VERSE_MATRIX_P * eyeVertex;
    center2D_gs = vec2(proj.x / proj.w, proj.y / proj.w);
    center2D_gs.x = 0.5f * (WIDTH + (center2D_gs.x * WIDTH) + (2.0f * X0));
    center2D_gs.y = 0.5f * (HEIGHT + (center2D_gs.y * HEIGHT) + (2.0f * Y0));

    // compute radiance from SH
    vec3 eyeDirection = normalize(eyeVertex.xyz / eyeVertex.w);
    //vec3 direction = transpose(mat3(osg_ViewMatrixInverse)) * eyeDirection;
    color_gs = vec4(computeRadianceFromSH(eyeDirection), alpha);
    gl_Position = proj;
}
