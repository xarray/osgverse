uniform vec2 InvScreenResolution;
VERSE_VS_IN vec4 color_gs[], covariance_gs[];
VERSE_VS_IN vec2 center2D_gs[];
VERSE_VS_OUT vec4 color, invCovariance;
VERSE_VS_OUT vec2 center2D;

mat2 inverseMat2(mat2 m)
{
    float det = m[0][0] * m[1][1] - m[0][1] * m[1][0]; mat2 inv;
    inv[0][0] = m[1][1] / det; inv[0][1] = -m[0][1] / det;
    inv[1][0] = -m[1][0] / det; inv[1][1] = m[0][0] / det;
    return inv;
}

void main()
{
    // we pass the inverse of the 2d covariance matrix to the pixel shader, to avoid doing a matrix inverse per pixel.
    mat2 cov2D = mat2(covariance_gs[0].xy, covariance_gs[0].zw);
    mat2 cov2Dinv = inverseMat2(cov2D);
    vec4 cov2Dinv4 = vec4(cov2Dinv[0], cov2Dinv[1]);

    // discard splats that end up outside of a guard band
    vec4 proj = gl_PositionIn[0]; vec3 ndcP = proj.xyz / proj.w;
    if (ndcP.z < 0.25 || ndcP.x > 2.0 || ndcP.x < -2.0 || ndcP.y > 2.0 || ndcP.y < -2.0) return;

    // compute 2d extents for the splat, using covariance matrix ellipse (https://cookierobotics.com/007/)
    float k = 3.5, a = cov2D[0][0], b = cov2D[0][1], c = cov2D[1][1];
    float apco2 = (a + c) / 2.0, amco2 = (a - c) / 2.0;
    float term = sqrt(amco2 * amco2 + b * b);
    float maj = apco2 + term, min = apco2 - term;

    float theta = (b == 0.0) ? ((a >= c) ? 0.0 : radians(90.0)) : atan(maj - a, b);
    float r1 = k * sqrt(maj), r2 = k * sqrt(min);
    vec2 majAxis = vec2(r1 * cos(theta), r1 * sin(theta));
    vec2 minAxis = vec2(r2 * cos(theta + radians(90.0)), r2 * sin(theta + radians(90.0)));

    vec2 offsets[4];
    offsets[0] = majAxis + minAxis; offsets[1] = -majAxis + minAxis;
    offsets[2] = majAxis - minAxis; offsets[3] = -majAxis - minAxis;

    float w = gl_PositionIn[0].w;
    for (int i = 0; i < 4; i++)
    {
        // transform offset back into clip space, and apply it to gl_Position.
        vec2 offset = offsets[i];
        offset.x *= (2.0 * InvScreenResolution.x) * w;
        offset.y *= (2.0 * InvScreenResolution.y) * w;
        //offset.x = (i == 1 || i == 3) ? -0.01 : 0.01; offset.y = (i == 2 || i == 3) ? -0.01 : 0.01;

        gl_Position = gl_PositionIn[0] + vec4(offset.x, offset.y, 0.0, 0.0);
        color = color_gs[0]; center2D = center2D_gs[0];
        invCovariance = cov2Dinv4;
        EmitVertex();
    }
    EndPrimitive();
}
