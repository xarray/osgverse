////////////////// COMMON
vec3 barycentricInTriangle(vec2 p, vec2 a, vec2 b, vec2 c);
vec4 tiling(vec2 uv, vec2 number);
mat2 rotationMatrix2D(float angle);
mat4 rotationMatrix3D(vec3 axis0, float angle);
vec2 rotateVector2(vec2 v, float angle);
vec3 rotateVector3(vec3 v, vec3 axis, float angle);
mat2 transposeMatrix(mat2 m)
mat3 transposeMatrix(mat3 m)
mat4 transposeMatrix(mat4 m)
mat2 inverseMatrix(mat2 m);
mat3 inverseMatrix(mat3 m);
mat4 inverseMatrix(mat4 m);
mat3 lookAtMatrix(vec3 origin, vec3 target, float roll);

////////////////// EASING
float backInOut(float t);
float bounceInOut(float t);
float circularInOut(float t);
float cubicInOut(float t);
float elasticInOut(float t);
float exponentialInOut(float t);
float quadraticInOut(float t);
float quarticInOut(float t);
float qinticInOut(float t);
