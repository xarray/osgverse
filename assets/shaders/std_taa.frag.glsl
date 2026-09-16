// Temporal anti-aliasing (TAA) resolve pass.
// The scene is rendered into the G-Buffer with a sub-pixel jittered projection every frame
// (see Pipeline::Stage::jitterProjection), and this pass blends the current color with the
// history color resampled from the previous frame:
// - The current pixel is unprojected to world space with the G-Buffer matrices of this frame
//   and projected again with the view-projection matrix of the previous frame;
// - Both matrix sets contain their own jitter, so the sub-pixel offsets are handled
//   implicitly and no extra jitter correction is needed here;
// - Motion of moving objects is not known (there is no velocity buffer yet), so such pixels
//   are protected by clamping the history into the neighborhood of the current frame. Static
//   geometry is reprojected exactly, and it is also why ghosting only appears on movers.
uniform sampler2D ColorBuffer, DepthBuffer, HistoryBuffer;
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v of the current frame
uniform mat4 PreviousViewProj;  // world-to-clip matrix used by the previous frame
uniform float HistoryWeight;  // 0.0: only the current frame, 1.0: only the history data
uniform vec2 InvScreenResolution;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

void main()
{
    vec2 uv0 = texCoord0.xy, texel = InvScreenResolution;
    vec3 color = VERSE_TEX2D(ColorBuffer, uv0).rgb;

    // Neighborhood of the current frame, used to clamp (and therefore validate) the history
    vec3 colorMin = color, colorMax = color;
    for (int y = -1; y <= 1; ++y)
    for (int x = -1; x <= 1; ++x)
    {
        vec3 c = VERSE_TEX2D(ColorBuffer, uv0 + vec2(float(x), float(y)) * texel).rgb;
        colorMin = min(colorMin, c); colorMax = max(colorMax, c);
    }

    // Rebuild the world vertex and reproject it to the previous frame. Background pixels
    // (e.g. the sky box) have no valid world position, but they are attached to the camera
    // and keep their screen position, so their history is found at the same UV. Handling
    // them in a different way from the pixels in front of them would make the sub-pixel
    // jitter of a silhouette visible: the same pixel would be accumulated on one frame and
    // not on the next one, which flickers along edges facing a bright background
    float depthValue = VERSE_TEX2D(DepthBuffer, uv0).r;
    vec2 prevUV = uv0;
    if (depthValue < 1.0)
    {
        vec4 ndcPos = vec4(uv0 * 2.0 - 1.0, depthValue * 2.0 - 1.0, 1.0);
        vec4 eyePos = GBufferMatrices[3] * ndcPos; eyePos /= eyePos.w;
        vec4 worldPos = GBufferMatrices[1] * eyePos;
        vec4 prevClip = PreviousViewProj * worldPos;
        prevUV = (prevClip.xy / prevClip.w) * 0.5 + vec2(0.5);
    }

    // Pixels outside of the screen have no history data at all: keep the current color
    float historyWeight = HistoryWeight;
    if (any(lessThan(prevUV, vec2(0.0))) || any(greaterThan(prevUV, vec2(1.0))))
        historyWeight = 0.0;

    // Clamp the history into the neighborhood of the current frame, which rejects most of the
    // ghosting introduced by moving objects (details above) and by wrong depth reprojection
    vec3 history = VERSE_TEX2D(HistoryBuffer, prevUV).rgb;
    history = clamp(history, colorMin, colorMax);
    fragData = vec4(mix(color, history, historyWeight), 1.0);
    VERSE_FS_FINAL(fragData);
}
