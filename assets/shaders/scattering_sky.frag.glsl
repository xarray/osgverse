uniform sampler2D sceneSampler;
uniform sampler2D glareSampler;
uniform sampler2D transmittanceSampler;
uniform sampler2D skyIrradianceSampler;
uniform sampler3D inscatterSampler;
uniform vec3 worldCameraPos, worldSunDir, origin;
uniform float hdrExposure, opaque;

uniform vec3 ColorAttribute;     // (Brightness, Saturation, Contrast)
uniform vec3 ColorBalance;       // (Cyan-Red, Magenta-Green, Yellow-Blue)
uniform int ColorBalanceMode;    // 0 - Shadow, 1 - Midtone, 2 - Highlight

VERSE_FS_IN vec3 dir;
VERSE_FS_IN vec3 relativeDir;
VERSE_FS_IN vec4 texCoord;
VERSE_FS_OUT vec4 fragColor;

#define SUN_INTENSITY 100.0
#define PLANET_RADIUS 6360000.0

#include "scattering.module.glsl"

vec3 hdr(vec3 L)
{
    L = L * hdrExposure;
    L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
    L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
    L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
    return L;
}

void main()
{
    vec3 WSD = worldSunDir;
    vec3 WCP = worldCameraPos;
    vec3 d = normalize(dir);
    vec3 sunColor = outerSunRadiance(relativeDir);
    fragColor.a = 1.0;
    
    vec3 extinction = vec3(1.0);
    vec3 inscatter = skyRadiance(WCP + origin, d, WSD, extinction, 0.0);
    vec3 finalColor = sunColor * extinction + inscatter;

    // Color grading work
    finalColor = colorBalanceFunc(finalColor, ColorBalance.x, ColorBalance.y, ColorBalance.z, ColorBalanceMode);
    finalColor = colorAdjustmentFunc(finalColor, ColorAttribute.x, ColorAttribute.y, ColorAttribute.z);
    
    vec4 scene = VERSE_TEX2D(sceneSampler, texCoord.st);
    fragColor.rgb = mix(hdr(finalColor), scene.rgb, scene.a);
    fragColor.rgb = mix(scene.rgb, fragColor.rgb, clamp(opaque, 0.0, 1.0));
    VERSE_FS_FINAL(fragColor);
}
