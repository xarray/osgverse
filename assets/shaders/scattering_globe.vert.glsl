uniform mat4 osg_ViewMatrix, osg_ViewMatrixInverse;
uniform sampler2D maskSampler;
uniform float oceanOpaque, underOcean;
VERSE_VS_IN vec4 osg_GlobeData;  // (default coord, skirt flag)
VERSE_VS_OUT vec3 vertexInWorld, normalInWorld;
VERSE_VS_OUT vec4 texCoord;
VERSE_VS_OUT float isSkirt;

void main()
{
    mat4 modelMatrix = osg_ViewMatrixInverse * VERSE_MATRIX_MV;
    vertexInWorld = vec3(modelMatrix * osg_Vertex);
    normalInWorld = normalize(vec3(osg_ViewMatrixInverse * vec4(VERSE_MATRIX_N * gl_Normal, 0.0)));
    texCoord = osg_MultiTexCoord0; texCoord.zw = osg_GlobeData.zw; isSkirt = osg_GlobeData.w;

    vec4 vertex = osg_Vertex, maskColor = VERSE_TEX2D(maskSampler, texCoord.st);
    if (maskColor.z < 0.5 && isSkirt > -0.5f &&  // if: mask is ocean + not skirt + not transparent + not under ocean
        oceanOpaque > 0.5 && underOcean > 0.0)   // then: should see ocean instead of under-sea ground
    {
        vec3 vertex0 = osg_GlobeData.xyz;
        gl_Position = VERSE_MATRIX_MVP * vec4(vertex0, 1.0);  // ocean plane position
    }
    else
        gl_Position = VERSE_MATRIX_MVP * vertex;  // real ground position
}
