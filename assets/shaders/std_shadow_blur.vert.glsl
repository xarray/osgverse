VERSE_VS_OUT vec4 texCoord0;

void main()
{
    // The blur runs on a full-screen quad whose UVs cover [0, 1] in the shadow-map texture.
    // The MVP is the standard ortho projection of the blur camera, so the geometry covers
    // every shadow-map texel exactly once
    texCoord0 = osg_MultiTexCoord0;
    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;
}