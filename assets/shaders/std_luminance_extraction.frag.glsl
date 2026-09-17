// ColorBuffer is the combined scene color (direct lighting, shadowed, plus emission) and
// AmbientBuffer the IBL contribution: together they are exactly what the tone mapping stage
// receives, so the exposure is calibrated on the same quantity as the displayed image.
// The bloom is left out on purpose: it is a post effect of that color, and metering it would
// couple the exposure to the bloom settings again
uniform sampler2D ColorBuffer, AmbientBuffer, DepthBuffer;
// 1.0 when the background of the color buffer holds valid content (the sky of the deferred
// pipeline), 0.0 when it is an empty background which must not take part in the average
uniform float IncludeBackground;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

// Luminance of a pixel is bounded before it enters the average: a few very bright pixels (a
// specular highlight, the sun, an emissive surface) must not be able to drive the metering of
// the whole frame, which is what happen when the luminance is unbounded
const float MAX_LUMINANCE = 10.0;

void main()
{
    // First level of the eye adaptation chain: it writes the luminance of the pixel, which the
    // following downsampling levels average, so that the last level holds the average luminance
    // of the frame. The arithmetic mean is used instead of the geometric one on purpose: the
    // geometric mean is dragged far below the median by the many unlit (almost black) pixels of
    // a typical interior scene, which makes the exposure far too high and washes the image out.
    // With the average, the dark pixels simply weight almost nothing while the metering still
    // follows the bright content of a bright scene.
    // The second component counts the pixels which really take part: the background (depth keeps
    // its maximum value) writes (0, 0) and is therefore excluded when it holds nothing, so that
    // the result does not depend on how much of the screen the scene covers
    vec2 uv0 = texCoord0.xy;
    float depthValue = VERSE_TEX2D(DepthBuffer, uv0).r * 2.0 - 1.0;
    if (IncludeBackground < 0.5 && depthValue >= 1.0)
        fragData = vec4(0.0, 0.0, 0.0, 1.0);
    else
    {
        vec3 color = VERSE_TEX2D(ColorBuffer, uv0).rgb + VERSE_TEX2D(AmbientBuffer, uv0).rgb;
        float lum = dot(color, vec3(0.2126, 0.7152, 0.0722));

        // A pixel which is not a number (an uninitialised buffer, a division by zero somewhere in
        // the lighting) is not metered at all: it is excluded exactly like a background pixel, so
        // that a single one of them can neither poison the average of the frame nor make the whole
        // measurement unusable
        if (lum == lum)
            fragData = vec4(min(lum, MAX_LUMINANCE), 1.0, 0.0, 1.0);
        else
            fragData = vec4(0.0, 0.0, 0.0, 1.0);
    }
    VERSE_FS_FINAL(fragData);
}
