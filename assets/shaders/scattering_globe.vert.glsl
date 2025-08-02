uniform mat4 osg_ViewMatrix, osg_ViewMatrixInverse;
uniform sampler2D maskSampler;
uniform float oceanOpaque, underOcean;
VERSE_VS_IN vec2 osg_GlobeData;
VERSE_VS_OUT vec3 normalInWorld;
VERSE_VS_OUT vec3 vertexInWorld;
VERSE_VS_OUT vec4 texCoord;

const float WGS84_EQUATOR = 6378137.0;
void main()
{
    mat4 modelMatrix = osg_ViewMatrixInverse * VERSE_MATRIX_MV;
    vertexInWorld = vec3(modelMatrix * osg_Vertex);
    normalInWorld = normalize(vec3(osg_ViewMatrixInverse * vec4(VERSE_MATRIX_N * gl_Normal, 0.0)));
    texCoord = osg_MultiTexCoord0;

    vec4 vertex = osg_Vertex, maskColor = VERSE_TEX2D(maskSampler, texCoord.st);
    if (maskColor.z < 0.5 && oceanOpaque > 0.5 && underOcean > 0.0)
    {
        vec3 vertex0 = normalize(vertexInWorld) * (WGS84_EQUATOR + osg_GlobeData.x + osg_GlobeData.y);
        gl_Position = VERSE_MATRIX_P * osg_ViewMatrix * vec4(vertex0, 1.0);
    }
    else
        gl_Position = VERSE_MATRIX_MVP * vertex;
}
