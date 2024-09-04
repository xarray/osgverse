#if VERSE_WEBGL1
#extension GL_OES_standard_derivatives : enable
#elif VERSE_WEBGL2
#extension GL_OES_standard_derivatives : enable
#endif

#ifndef PI
#define PI 3.141592653589793
#endif

#ifndef HALF_PI
#define HALF_PI 1.5707963267948966
#endif

////////////////// COMMON

vec3 barycentricInTriangle(vec2 p, vec2 a, vec2 b, vec2 c)
{
    float l0 = ((b.y-c.y)*(p.x-c.x) + (c.x-b.x)*(p.y-c.y))
             / ((b.y-c.y)*(a.x-c.x)+(c.x-b.x)*(a.y-c.y));
    float l1 = ((c.y-a.y)*(p.x-c.x)+(a.x-c.x)*(p.y-c.y))
             / ((b.y-c.y)*(a.x-c.x)+(c.x-b.x)*(a.y-c.y));
    return vec3(l0, l1, 1.0 - l0 - l1);
}

mat2 rotationMatrix2D(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return mat2(c, -s, s, c);
}

mat4 rotationMatrix3D(vec3 axis0, float angle)
{
    vec3 axis = normalize(axis0);
    float s = sin(angle), c = cos(angle);
    float oc = 1.0 - c;
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

vec2 rotateVector2(vec2 v, float angle)
{
    return rotationMatrix2D(angle) * v;
}

vec3 rotateVector3(vec3 v, vec3 axis, float angle)
{
    return (rotationMatrix3D(axis, angle) * vec4(v, 1.0)).xyz;
}

mat2 transposeMatrix(mat2 m)
{
    return mat2(m[0][0], m[1][0], m[0][1], m[1][1]);
}

mat3 transposeMatrix(mat3 m)
{
    return mat3(m[0][0], m[1][0], m[2][0],
                m[0][1], m[1][1], m[2][1],
                m[0][2], m[1][2], m[2][2]);
}

mat4 transposeMatrix(mat4 m)
{
    return mat4(m[0][0], m[1][0], m[2][0], m[3][0],
                m[0][1], m[1][1], m[2][1], m[3][1],
                m[0][2], m[1][2], m[2][2], m[3][2],
                m[0][3], m[1][3], m[2][3], m[3][3]);
}

mat2 inverseMatrix(mat2 m)
{
    return mat2(m[1][1], -m[0][1], -m[1][0], m[0][0]) / (m[0][0]*m[1][1] - m[0][1]*m[1][0]);
}

mat3 inverseMatrix(mat3 m)
{
    float a00 = m[0][0], a01 = m[0][1], a02 = m[0][2];
    float a10 = m[1][0], a11 = m[1][1], a12 = m[1][2];
    float a20 = m[2][0], a21 = m[2][1], a22 = m[2][2];
    float b01 = a22 * a11 - a12 * a21;
    float b11 = -a22 * a10 + a12 * a20;
    float b21 = a21 * a10 - a11 * a20;
    float det = a00 * b01 + a01 * b11 + a02 * b21;
    return mat3(b01, (-a22 * a01 + a02 * a21), (a12 * a01 - a02 * a11),
                b11, (a22 * a00 - a02 * a20), (-a12 * a00 + a02 * a10),
                b21, (-a21 * a00 + a01 * a20), (a11 * a00 - a01 * a10)) / det;
}

mat4 inverseMatrix(mat4 m)
{
    float a00 = m[0][0], a01 = m[0][1], a02 = m[0][2], a03 = m[0][3],
          a10 = m[1][0], a11 = m[1][1], a12 = m[1][2], a13 = m[1][3],
          a20 = m[2][0], a21 = m[2][1], a22 = m[2][2], a23 = m[2][3],
          a30 = m[3][0], a31 = m[3][1], a32 = m[3][2], a33 = m[3][3],
          b00 = a00 * a11 - a01 * a10,
          b01 = a00 * a12 - a02 * a10,
          b02 = a00 * a13 - a03 * a10,
          b03 = a01 * a12 - a02 * a11,
          b04 = a01 * a13 - a03 * a11,
          b05 = a02 * a13 - a03 * a12,
          b06 = a20 * a31 - a21 * a30,
          b07 = a20 * a32 - a22 * a30,
          b08 = a20 * a33 - a23 * a30,
          b09 = a21 * a32 - a22 * a31,
          b10 = a21 * a33 - a23 * a31,
          b11 = a22 * a33 - a23 * a32,
          det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
  return mat4(a11 * b11 - a12 * b10 + a13 * b09, a02 * b10 - a01 * b11 - a03 * b09,
              a31 * b05 - a32 * b04 + a33 * b03, a22 * b04 - a21 * b05 - a23 * b03,
              a12 * b08 - a10 * b11 - a13 * b07, a00 * b11 - a02 * b08 + a03 * b07,
              a32 * b02 - a30 * b05 - a33 * b01, a20 * b05 - a22 * b02 + a23 * b01,
              a10 * b10 - a11 * b08 + a13 * b06, a01 * b08 - a00 * b10 - a03 * b06,
              a30 * b04 - a31 * b02 + a33 * b00, a21 * b02 - a20 * b04 - a23 * b00,
              a11 * b07 - a10 * b09 - a12 * b06, a00 * b09 - a01 * b07 + a02 * b06,
              a31 * b01 - a30 * b03 - a32 * b00, a20 * b03 - a21 * b01 + a22 * b00) / det;
}

mat3 lookAtMatrix(vec3 origin, vec3 target, float roll)
{
    vec3 rr = vec3(sin(roll), cos(roll), 0.0);
    vec3 ww = normalize(target - origin);
    vec3 uu = normalize(cross(ww, rr));
    vec3 vv = normalize(cross(uu, ww));
    return mat3(uu, vv, ww);
}

////////////////// EASING

float backInOut(float t)
{
    float f = (t < 0.5) ? (2.0 * t) : (1.0 - (2.0 * t - 1.0));
    float g = pow(f, 3.0) - f * sin(f * PI);
    return (t < 0.5) ? (0.5 * g) : (0.5 * (1.0 - g) + 0.5);
}

float bounceOut(float t)
{
    const float a = 4.0 / 11.0;
    const float b = 8.0 / 11.0;
    const float c = 9.0 / 10.0;
    const float ca = 4356.0 / 361.0;
    const float cb = 35442.0 / 1805.0;
    const float cc = 16061.0 / 1805.0;
    float t2 = t * t;
    return (t < a) ? (7.5625 * t2)
                   : (t < b) ? (9.075 * t2 - 9.9 * t + 3.4)
                             : (t < c) ? (ca * t2 - cb * t + cc)
                                       : (10.8 * t * t - 20.52 * t + 10.72);
}

float bounceInOut(float t)
{
    return (t < 0.5) ? (0.5 * (1.0 - bounceOut(1.0 - t * 2.0))) : (0.5 * bounceOut(t * 2.0 - 1.0) + 0.5);
}

float circularInOut(float t)
{
    return (t < 0.5) ? (0.5 * (1.0 - sqrt(1.0 - 4.0 * t * t)))
                     : (0.5 * (sqrt((3.0 - 2.0 * t) * (2.0 * t - 1.0)) + 1.0));
}

float cubicInOut(float t)
{
    return (t < 0.5) ? (4.0 * t * t * t)
                     : (0.5 * pow(2.0 * t - 2.0, 3.0) + 1.0);
}

float elasticInOut(float t)
{
    return (t < 0.5) ? (0.5 * sin(+13.0 * HALF_PI * 2.0 * t) * pow(2.0, 10.0 * (2.0 * t - 1.0)))
                     : (0.5 * sin(-13.0 * HALF_PI * ((2.0 * t - 1.0) + 1.0)) * pow(2.0, -10.0 * (2.0 * t - 1.0)) + 1.0);
}

float exponentialInOut(float t)
{
    return (t == 0.0 || t == 1.0) ? t : (t < 0.5) ? (+0.5 * pow(2.0, (20.0 * t) - 10.0))
                                                  : (-0.5 * pow(2.0, 10.0 - (t * 20.0)) + 1.0);
}

float quadraticInOut(float t)
{
    float p = 2.0 * t * t;
    return (t < 0.5) ? p : (-p + (4.0 * t) - 1.0);
}

float quarticInOut(float t)
{
    return (t < 0.5) ? (+8.0 * pow(t, 4.0)) : (-8.0 * pow(t - 1.0, 4.0) + 1.0);
}

float qinticInOut(float t)
{
    return (t < 0.5) ? (+16.0 * pow(t, 5.0)) : (-0.5 * pow(2.0 * t - 2.0, 5.0) + 1.0);
}
