#ifdef VERSE_DEFERRED_SKY
// Deferred shading variant: the sky is drawn by a full-screen pass inside the deferred
// pipeline (after the shadowing stage and before bloom / tone mapping). The sampling
// direction is therefore rebuilt from the screen position, and pixels in front of the
// sky are skipped as they are already handled by the deferred lighting. The color is
// kept in linear space, as the tone mapping stage will encode it afterwards
uniform sampler2D DepthBuffer;
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v of the G-Buffer camera
VERSE_FS_IN vec4 texCoord0;  // screen UV, see std_forward_render.frag.glsl
#else
VERSE_FS_IN vec3 texCoord;  // direction of the sky box vertex, see skybox.vert.glsl
#endif

#if VERSE_CUBEMAP_SKYBOX
uniform samplerCube SkyTexture;
#else
uniform sampler2D SkyTexture;
#endif
VERSE_FS_OUT vec4 fragData;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 sphericalUV(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan; uv += vec2(0.5);
    return clamp(uv, vec2(0.0), vec2(1.0));
}

void main()
{
#ifdef VERSE_DEFERRED_SKY
    // Background pixels are the ones nothing was rendered to by the G-Buffer, so they
    // keep the maximum depth value
    float depthValue = VERSE_TEX2D(DepthBuffer, texCoord0.xy).r;
    if (depthValue < 1.0) discard;

    // Rebuild the world vertex of this pixel and use the direction from the camera to it
    // as the sampling direction, which is the one of an infinite sky box
    vec4 ndcPos = vec4(texCoord0.xy * 2.0 - 1.0, depthValue * 2.0 - 1.0, 1.0);
    vec4 eyePos = GBufferMatrices[3] * ndcPos; eyePos /= eyePos.w;
    vec4 worldPos = GBufferMatrices[1] * eyePos;
    vec3 direction = normalize(worldPos.xyz - GBufferMatrices[1][3].xyz);

#  if VERSE_CUBEMAP_SKYBOX
    fragData = VERSE_TEXCUBE(SkyTexture, direction);
#  else
    // An equirectangular sky map is stored with its poles along the X axis, so the forward
    // sky box rotates the sampling direction by -90 degrees about X before building the
    // spherical UV (see SkyBox::setEnvironmentMap() with keepYAxisUp = false, which is what
    // the applications use for 2D sky maps). Applying the same rotation here is required for
    // both sky implementations to show the environment map in the same orientation:
    // rotate(-PI/2, X) maps (x, y, z) to (x, z, -y). Cubemaps are already Y-up
    vec3 mapDir = vec3(direction.x, direction.z, -direction.y);
    fragData = VERSE_TEX2D(SkyTexture, sphericalUV(mapDir));
#  endif
    fragData.a = 1.0;  // the stage is alpha-blended, so the sky must always be opaque
#else
#  if VERSE_CUBEMAP_SKYBOX
    vec4 skyColor = VERSE_TEXCUBE(SkyTexture, texCoord);
#else
    vec4 skyColor = VERSE_TEX2D(SkyTexture, sphericalUV(texCoord));
#endif
    fragData = pow(skyColor, vec4(1.0 / 2.2));
#endif
    VERSE_FS_FINAL(fragData);
}
