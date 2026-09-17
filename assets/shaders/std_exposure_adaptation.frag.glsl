// Eye adaptation (auto exposure) of the standard pipeline. It is done entirely on the GPU: the
// setup creates two 1x1 stages which ping-pong their result, and each of them reads what the
// other one wrote during the previous frame (see the "EyeAdaptation0/1" stages of
// PipelineStandard.cpp). Only one of the two is drawn per frame, so a state is never read while
// it is written, and no CPU readback (nor any synchronization point) is needed to adapt.
uniform sampler2D LuminanceBuffer, HistoryBuffer;
uniform vec2 MeteringSize;         // size, in texels, of the metering buffer bound above
uniform float Initialized;         // 0 during the very first frame: there is no state to read yet
uniform float KeyValue, Compensation, DeltaTime;
uniform float SpeedIncrease, SpeedDecrease;   // 1/second, in log2 space
uniform float MinLogExposure, MaxLogExposure;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

// The exposure is stored as its log2, encoded in [0,1] over this range, so that the same code
// works with the half float buffer of the desktop builds (a precision of about 0.02 EV there)
// and with the INT8 one the embedded builds have to use, as half float render targets are not
// guaranteed to be color-renderable there. Keep this range in sync with the decoding in
// std_tonemapping.frag.glsl and with EXPOSURE_ENCODE_MIN/MAX in PipelineStandard.cpp, which
// clamps the exposure limits to it
const float ENCODE_MIN = -10.0, ENCODE_MAX = 6.0;

// The metering buffer is the last level of the luminance chain. That chain stops as soon as its
// scaled height would fall to 2 texels, which leaves the last level 2 to 4 texels high while its
// width follows the aspect ratio of the display (see the loop which creates it in
// PipelineStandard.cpp). Its levels are half float on the desktop builds and 8 bit on the embedded
// ones, where the luminances it averages are already below 1 (the scene color of those builds is
// stored in 8 bit buffers), so neither of them loses anything here
const int MAX_TAPS_X = 16, MAX_TAPS_Y = 4;

void main()
{
    // Average of the metering buffer: each of its texels already holds the average of the pixels
    // it covers, so the average of the whole buffer is the average of the frame. Sampling every
    // texel at its center makes the result exact whatever the size of the buffer, which a couple
    // of bilinear taps would not be (the buffer is typically 3x2 texels, too small for a
    // downsampling stage to guarantee anything)
    vec2 invSize = 1.0 / max(MeteringSize, vec2(1.0)), sum = vec2(0.0);
    for (int y = 0; y < MAX_TAPS_Y; ++y)
    {
        float fy = float(y) + 0.5;
        for (int x = 0; x < MAX_TAPS_X; ++x)
        {
            float fx = float(x) + 0.5;
            if (fx < MeteringSize.x && fy < MeteringSize.y)
                sum += VERSE_TEX2D(LuminanceBuffer, vec2(fx, fy) * invSize).rg;
        }
    }

    // Exposure of this frame, in log2. Each texel of the metering buffer holds the average
    // luminance of the pixels it covers in its red channel and the share of the metered ones in
    // its green one: both are averages, so the green channel is a coverage in [0,1] and NOT a
    // pixel count (see std_luminance_extraction.frag.glsl and the downsampling levels which keep
    // averaging it). Summing them over the whole level therefore gives the average luminance of
    // the metered pixels as their ratio, and the exposure which brings that average to the target
    // middle gray is the key value divided by it. A null count means that nothing at all was
    // metered, which is what a frame whose background holds nothing measures
    bool valid = (sum.g > 0.0001 && sum.r > 0.0);
    float target = 0.0;
    if (valid)
    {
        float lumAvg = sum.r / sum.g;
        target = log2(KeyValue) + Compensation - log2(max(lumAvg, 0.000001));
    }

    // A metering which is not a number can not be trusted either (an uninitialised buffer, a
    // division by zero somewhere in the lighting): such a frame keeps the state as well
    valid = valid && (sum.r == sum.r) && (sum.g == sum.g);
    target = clamp(target, MinLogExposure, MaxLogExposure);

    // State written by the previous frame: the exposure in the red channel, and whether that
    // value is meaningful in the green one. A buffer which has never been drawn into holds
    // anything, and the state of the very first frame can not be used either as the two stages of
    // the pair ping-pong it (the one drawn then reads a buffer which was never written). Anything
    // which is not a number is rejected the same way. Without any state to continue from, the
    // adaptation starts at a neutral exposure of 1, which is what the pipeline displayed before
    // it had anything to adapt to: starting from the darkest exposure instead would turn every
    // frame which can not be metered yet into a black image
    vec4 history = VERSE_TEX2D(HistoryBuffer, vec2(0.5, 0.5));
    bool usable = (Initialized > 0.5) && (history.g > 0.5) && (history.r == history.r);
    float previous = clamp(usable ? mix(ENCODE_MIN, ENCODE_MAX, history.r) : 0.0,
                           MinLogExposure, MaxLogExposure);

    // Temporal adaptation: an exponential approach in log2 space, which makes the transition
    // perceptually uniform, with a speed which is not the same whether the exposure has to
    // increase or to decrease. The very first frame has no state to continue from and therefore
    // takes the target as it is, so that the image is exposed correctly right away
    float adapted = previous;
    if (valid)
    {
        if (usable)
        {
            float speed = (target > previous) ? SpeedIncrease : SpeedDecrease;
            adapted = mix(previous, target, 1.0 - exp(-speed * DeltaTime));
        }
        else adapted = target;
    }

    // Keep the previous state, and its validity, when nothing was measured this frame
    float state = (valid || usable) ? 1.0 : 0.0;
    float encoded = clamp((adapted - ENCODE_MIN) / (ENCODE_MAX - ENCODE_MIN), 0.0, 1.0);
    fragData = vec4(encoded, state, 0.0, 1.0);
    VERSE_FS_FINAL(fragData);
}
