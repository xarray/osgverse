////////////////// COMMON
float VERSE_smoothMin(float a, float b, float k);
vec3 VERSE_approximateFaceNormal(vec3 eyePos);
vec4 VERSE_tiling(vec2 uv, vec2 number);
vec3 VERSE_convertRGB2HSV(vec3 rgb);
vec3 VERSE_convertHSV2RGB(vec3 hsv);
vec3 VERSE_convertHSV2RGB_Smooth(vec3 hsv);
vec3 VERSE_lerpHSV(vec3 hsv1, vec3 hsv2, float rate);
mat2 VERSE_rotationMatrix2D(float angle);
mat4 VERSE_rotationMatrix3D(vec3 axis0, float angle);
vec2 VERSE_rotateVector2(vec2 v, float angle);
vec3 VERSE_rotateVector3(vec3 v, vec3 axis, float angle);
mat2 VERSE_transposeMatrix(mat2 m);
mat3 VERSE_transposeMatrix(mat3 m);
mat4 VERSE_transposeMatrix(mat4 m);
mat2 VERSE_inverseMatrix(mat2 m);
mat3 VERSE_inverseMatrix(mat3 m);
mat4 VERSE_inverseMatrix(mat4 m);

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
