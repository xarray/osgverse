uniform sampler2D BrightnessCombinedBuffer;
uniform float BloomFactor;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

const float noiseSeed = 20.0;
const float noiseStrength = 0.3;

float hash(vec2 p)
{
    float h = dot(p, vec2(127.1, 311.7));
    return -1.0 + 2.0 * fract(sin(h) * 43758.5453123);
}

float noise(in vec2 p)
{
    vec2 i = floor(p), f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p)
{
    float f = 0.0;
    f += 0.5000 * noise(p); p *= 2.02;
    f += 0.2500 * noise(p); p *= 2.03;
    f += 0.1250 * noise(p); p *= 2.01;
    f += 0.0625 * noise(p); p *= 2.04;
    f /= 0.9375; return f;
}

float luminance(vec3 color) {
    return dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
}

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec2 uv0 = texCoord0.xy;
    vec4 color = VERSE_TEX2D(BrightnessCombinedBuffer, uv0);

    // Film grain proportional to the bloom brightness. The bloom buffer is linear HDR now, so
    // its luminance has to be clamped: otherwise a highlight of e.g. 50.0 would add 15.0 worth
    // of noise instead of a subtle grain (values <= 1.0 keep the previous behaviour)
    float f = fbm(vec2(uv0 * noiseSeed));
    color.rgb += vec3(noiseStrength * min(luminance(color.rgb), 1.0) * abs(f));
    fragData = vec4(color.rgb * BloomFactor, 1.0);
    VERSE_FS_FINAL(fragData);
}
