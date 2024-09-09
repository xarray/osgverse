////////////////// COMMON
vec3 VERSE_barycentricInTriangle(vec2 p, vec2 a, vec2 b, vec2 c);
vec4 VERSE_tiling(vec2 uv, vec2 number);
mat2 VERSE_rotationMatrix2D(float angle);
mat4 VERSE_rotationMatrix3D(vec3 axis0, float angle);
vec2 VERSE_rotateVector2(vec2 v, float angle);
vec3 VERSE_rotateVector3(vec3 v, vec3 axis, float angle);
mat2 VERSE_transposeMatrix(mat2 m)
mat3 VERSE_transposeMatrix(mat3 m)
mat4 VERSE_transposeMatrix(mat4 m)
mat2 VERSE_inverseMatrix(mat2 m);
mat3 VERSE_inverseMatrix(mat3 m);
mat4 VERSE_inverseMatrix(mat4 m);
mat3 VERSE_lookAtMatrix(vec3 origin, vec3 target, float roll);

////////////////// EASING
float VERSE_backInOut(float t);
float VERSE_bounceInOut(float t);
float VERSE_circularInOut(float t);
float VERSE_cubicInOut(float t);
float VERSE_elasticInOut(float t);
float VERSE_exponentialInOut(float t);
float VERSE_quadraticInOut(float t);
float VERSE_quarticInOut(float t);
float VERSE_qinticInOut(float t);
