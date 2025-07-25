uniform sampler2D sceneSampler, maskSampler;
uniform sampler2D glareSampler;
uniform sampler2D transmittanceSampler;
uniform sampler2D skyIrradianceSampler;
uniform sampler3D inscatterSampler;
uniform vec3 worldCameraPos, worldSunDir, origin;
uniform vec3 sunColorScale, skyColorScale;
uniform float hdrExposure, globalOpaque;

uniform vec3 ColorAttribute;     // (Brightness, Saturation, Contrast)
uniform vec3 ColorBalance;       // (Cyan-Red, Magenta-Green, Yellow-Blue)
uniform int ColorBalanceMode;    // 0 - Shadow, 1 - Midtone, 2 - Highlight

VERSE_FS_IN vec3 normalInWorld;
VERSE_FS_IN vec3 vertexInWorld;
VERSE_FS_IN vec4 texCoord, baseColor;

#ifdef VERSE_GLES3
layout(location = 0) VERSE_FS_OUT vec4 fragColor;
layout(location = 1) VERSE_FS_OUT vec4 fragOrigin;
#endif

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
    vec3 WSD = worldSunDir, WCP = worldCameraPos;
    vec3 P = vertexInWorld, N = normalInWorld;// normalize(P);
    P = N * (length(P) * 0.99);  // FIXME

    float cTheta = dot(N, WSD); vec3 sunL, skyE;
    sunRadianceAndSkyIrradiance(P, N, WSD, sunL, skyE);
    
    vec4 groundColor = VERSE_TEX2D(sceneSampler, texCoord.st) * baseColor;
    vec4 maskColor = VERSE_TEX2D(maskSampler, texCoord.st);
    groundColor.rgb *= max((sunL * sunColorScale * max(cTheta, 0.0) + skyE) / 3.14159265, vec3(0.1));
    groundColor.a *= clamp(globalOpaque, 0.0, 1.0);
    
    vec3 extinction = vec3(1.0);
    vec3 inscatter = inScattering(WCP, P, WSD, extinction, 0.0);
    vec3 compositeColor = groundColor.rgb * extinction + inscatter * skyColorScale;
    vec4 finalColor = vec4(hdr(compositeColor), groundColor.a);

    // Color grading work
    finalColor.rgb = colorBalanceFunc(finalColor.rgb, ColorBalance.x, ColorBalance.y, ColorBalance.z, ColorBalanceMode);
    finalColor.rgb = colorAdjustmentFunc(finalColor.rgb, ColorAttribute.x, ColorAttribute.y, ColorAttribute.z);

#ifdef VERSE_GLES3
    fragColor/*Atmospheric Color*/ = finalColor;
    fragOrigin/*Mask Color*/ = maskColor;
#else
    gl_FragData[0]/*Atmospheric Color*/ = finalColor;
    gl_FragData[1]/*Mask Color*/ = maskColor;
#endif
}
