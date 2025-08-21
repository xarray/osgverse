uniform sampler2D SceneSampler;
uniform sampler2D TransmittanceSampler;
uniform sampler2D SkyIrradianceSampler;
uniform sampler3D InscatterSampler;
uniform sampler2D GlareSampler;
uniform vec3 WorldCameraPos, WorldCameraLLA;
uniform vec3 WorldSunDir, EarthOrigin;
uniform float HdrExposure, GlobalOpaque;
uniform float osg_SimulationTime;

VERSE_FS_IN vec3 dir;
VERSE_FS_IN vec3 relativeDir;
VERSE_FS_IN vec4 texCoord;
VERSE_FS_OUT vec4 fragColor;

#define SUN_INTENSITY 100.0
#define PLANET_RADIUS 6360000.0
#include "scattering.module.glsl"

//////////////////// From https://www.shadertoy.com/view/tllfRX
#define NUM_LAYERS 4.0
#define TAU 6.28318
#define PI 3.141592
#define Velocity 0.0   // modified value to increse or decrease speed
#define StarGlow 0.0075
#define StarSize 0.1
#define CanvasView 20.0

float Star(vec2 uv, float flare)
{
    float d = length(uv); float m = sin(StarGlow * 1.2) / d;
    float rays = max(0.0, 0.5 - abs(uv.x * uv.y * 100000.0));
    m += (rays * flare) * 2.0; m *= smoothstep(1.0, 0.1, d); return m;
}

float Hash21(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32); return fract(p.x * p.y);
}

vec3 StarLayer(vec2 uv, float time)
{
    vec3 col = vec3(0); vec2 gv = fract(uv), id = floor(uv);
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            vec2 offs = vec2(x, y); float n = Hash21(id + offs); float size = fract(n);
            float star = Star(gv - offs - vec2(n, fract(n * 34.0)) + 0.5, smoothstep(0.1, 0.9, size) * 0.46);
            vec3 color = sin(vec3(0.2, 0.3, 0.9) * fract(n * 2345.2) * TAU) * 0.25 + 0.75;
            color = color * vec3(0.9, 0.59, 0.9 + size);
            star *= sin(time * 0.6 + n * TAU) * 0.5 + 0.5; col += star * size * color;
        }
    }
    return col;
}

vec3 galaxyImage(in vec2 uv, in float time)
{
    vec2 M = vec2(0);
    //M -= vec2(M.x + sin(time * 0.22), M.y - cos(time * 0.22));
    //M += (iMouse.xy - iResolution.xy * 0.5) / iResolution.y;
    M += vec2(WorldCameraLLA.y, WorldCameraLLA.x) * 2.0;

    float t = time * Velocity; vec3 col = vec3(0.0);
    for (float i = 0.0; i < 1.0; i += 1.0 / NUM_LAYERS)
    {
        float depth = fract(i + t); float scale = mix(CanvasView, 0.5, depth);
        float fade = depth * smoothstep(1.0, 0.9, depth);
        col += StarLayer(uv * scale + i * 453.2 - time * 0.05 + M, time) * fade;
    }
    return col;
}
////////////////////

vec3 hdr(vec3 L)
{
    L = L * HdrExposure;
    L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
    L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
    L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
    return L;
}

void main()
{
    vec4 scene = VERSE_TEX2D(SceneSampler, texCoord.st);
    vec3 WSD = WorldSunDir, WCP = WorldCameraPos;
    vec3 d = normalize(dir), sunColor = outerSunRadiance(relativeDir);
    fragColor.a = 1.0;
    
    vec3 extinction = vec3(1.0);
    vec3 inscatter = skyRadiance(WCP + EarthOrigin, d, WSD, extinction, 0.0);
    vec3 finalColor = sunColor * extinction + inscatter;
    if (scene.a < 0.01)
    {
        vec3 galaxy = galaxyImage(texCoord.st - vec2(0.5), osg_SimulationTime * 0.1);
        finalColor.rgb = mix(galaxy, finalColor.rgb, length(finalColor));
    }

    fragColor.rgb = mix(hdr(finalColor), scene.rgb, scene.a);
    fragColor.rgb = mix(scene.rgb, fragColor.rgb, clamp(GlobalOpaque, 0.0, 1.0));
    VERSE_FS_FINAL(fragColor);
}
