////////////////// COMMON
float smoothMin(float a, float b, float k);
vec3 approximateFaceNormal(vec3 eyePos);
vec4 tiling(vec2 uv, vec2 number);
vec3 convertRGB2HSV(vec3 rgb);
vec3 convertHSV2RGB(vec3 hsv);
vec3 convertHSV2RGB_Smooth(vec3 hsv);
vec3 lerpHSV(vec3 hsv1, vec3 hsv2, float rate);
mat2 rotationMatrix2D(float angle);
mat4 rotationMatrix3D(vec3 axis0, float angle);
vec2 rotateVector2(vec2 v, float angle);
vec3 rotateVector3(vec3 v, vec3 axis, float angle);
mat2 transposeMatrix(mat2 m);
mat3 transposeMatrix(mat3 m);
mat4 transposeMatrix(mat4 m);
mat2 inverseMatrix(mat2 m);
mat3 inverseMatrix(mat3 m);
mat4 inverseMatrix(mat4 m);

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
