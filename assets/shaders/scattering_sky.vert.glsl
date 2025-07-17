uniform mat4 cameraToWorld;
uniform mat4 screenToCamera;
uniform vec3 worldCameraPos;
uniform vec3 worldSunDir;
VERSE_VS_OUT vec3 dir;
VERSE_VS_OUT vec3 relativeDir;
VERSE_VS_OUT vec4 texCoord;

void main()
{
    vec4 position = VERSE_MATRIX_MVP * osg_Vertex;
    position.z = 0.9999999;
    texCoord = osg_MultiTexCoord0;
    
    // construct a rotation that transforms sundir to (0,0,1);
    vec3 WSD = worldSunDir;
    float theta = acos(WSD.z);
    float phi = atan(WSD.y, WSD.x);
    mat3 rz = mat3(cos(phi), -sin(phi), 0.0, sin(phi), cos(phi), 0.0, 0.0, 0.0, 1.0);
    mat3 ry = mat3(cos(theta), 0.0, sin(theta), 0.0, 1.0, 0.0, -sin(theta), 0.0, cos(theta));
    
    // apply this rotation to view dir to get relative viewdir
    dir = vec3(cameraToWorld * vec4((screenToCamera * position).xyz, 0.0));
    relativeDir = (ry * rz) * dir;
    gl_Position = position;
}
