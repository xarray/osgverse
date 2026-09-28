VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

// Source texture: the cast output for the horizontal pass, the intermediate for the vertical one.
// Both contain the same per-fragment moments (already warped to VSM/EVSM space by the cast stage),
// so the same box filter works for both modes
uniform sampler2D ShadowMap;

// .x  radius of the box filter in shadow-map texels. The kernel spans (0.5 - radius) ..
//     (0.5 + radius) in kernel space, so a radius of filterSize * 0.5 integrates to exactly
//     filterSize unit weights (see GetSampleWeight() below)
// .y  1.0 to blur along U (horizontal pass), 0.0 to blur along V (vertical pass). Using a uniform
//     for the direction instead of a per-stage #define lets both passes share one compiled
//     program object and keeps the pass behavior independent of the state-set define timing
// .z  unused
// .w  unused
uniform vec4 ShadowFilterAttribs;

uniform vec2 InvShadowMapSize;

// Truncated box-filter weight for one sample (DiligentFX ShadowConversions.fx GetSampleWeight):
// the kernel extends from (0.5 - radius) to (0.5 + radius), so a sample at integer offset i
// covers [i, i+1] in kernel space and is clipped to that intersection
float GetSampleWeight(int i, float radius)
{
    float lo = max(float(i),     min(0.5 - radius, 0.0));
    float hi = min(float(i) + 1.0, max(0.5 + radius, 1.0));
    return hi - lo;
}

void main()
{
    int halfWidth = int(floor(ShadowFilterAttribs.x + 0.5));
    float horizontal = ShadowFilterAttribs.y;
    vec4 sum = vec4(0.0);
    float totalWeight = 0.0;

    for (int i = -halfWidth; i <= halfWidth; ++i)
    {
        float w = GetSampleWeight(i, ShadowFilterAttribs.x);
        vec2 uv = texCoord0.xy + vec2(float(i) * InvShadowMapSize.x * horizontal,
                                      float(i) * InvShadowMapSize.y * (1.0 - horizontal));
        sum += VERSE_TEX2D(ShadowMap, uv) * w;
        totalWeight += w;
    }

    fragData = sum / totalWeight;
    VERSE_FS_FINAL(fragData);
}
