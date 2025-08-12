uniform sampler2D sceneSampler, maskSampler, extraLayerSampler;
uniform sampler2D transmittanceSampler;
uniform sampler2D skyIrradianceSampler;
uniform sampler3D inscatterSampler;
uniform sampler2D glareSampler;
uniform vec4 UvOffset1, UvOffset2, UvOffset3;
uniform vec3 worldCameraPos, worldSunDir, origin;
uniform vec3 sunColorScale, skyColorScale;
uniform float hdrExposure, globalOpaque;

uniform vec4 clipPlane0, clipPlane1, clipPlane2;

uniform vec3 ColorAttribute;     // (Brightness, Saturation, Contrast)
uniform vec3 ColorBalance;       // (Cyan-Red, Magenta-Green, Yellow-Blue)
uniform int ColorBalanceMode;    // 0 - Shadow, 1 - Midtone, 2 - Highlight

VERSE_FS_IN vec3 vertexInWorld, normalInWorld;
VERSE_FS_IN vec4 texCoord;

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
    vec4 worldPos = vec4(vertexInWorld, 1.0);
    float clipD0 = dot(worldPos, clipPlane0), clipD1 = dot(worldPos, clipPlane1), clipD2 = dot(worldPos, clipPlane2);
    if (clipD0 > 0.0 && clipD1 > 0.0 && clipD2 > 0.0)
    {
#ifdef VERSE_GLES3
        fragColor/*Atmospheric Color*/ = vec4(0.0);
        fragOrigin/*Mask Color*/ = vec4(0.0, 0.0, 1.0, 1.0);
#else
        gl_FragData[0]/*Atmospheric Color*/ = vec4(0.0);
        gl_FragData[1]/*Mask Color*/ = vec4(0.0, 0.0, 1.0, 1.0);
#endif
        return;
    }

    vec4 groundColor = VERSE_TEX2D(sceneSampler, texCoord.st * UvOffset1.zw + UvOffset1.xy);
    vec4 layerColor = VERSE_TEX2D(extraLayerSampler, texCoord.st * UvOffset3.zw + UvOffset3.xy);
    groundColor.rgb = mix(groundColor.rgb, layerColor.rgb, layerColor.a);

    // Mask color: r = aspect, g = slope, b = mask (0 - 0.5: land, 0.5 - 1: ocean)
    vec2 uv = texCoord.xy * UvOffset2.zw + UvOffset2.xy;
    vec4 maskColor = VERSE_TEX2D(maskSampler, uv.st); float off = 0.002;
    vec4 maskValue = vec4(maskColor.z, maskColor.z, maskColor.z, maskColor.a);
    maskColor += VERSE_TEX2D(maskSampler, uv.st + vec2(-off, 0.0));
    maskColor += VERSE_TEX2D(maskSampler, uv.st + vec2(off, 0.0));
    maskColor += VERSE_TEX2D(maskSampler, uv.st + vec2(0.0, -off));
    maskColor += VERSE_TEX2D(maskSampler, uv.st + vec2(0.0, off));
    maskColor += VERSE_TEX2D(maskSampler, uv.st + vec2(-off, -off));
    maskColor += VERSE_TEX2D(maskSampler, uv.st + vec2(off, -off));
    maskColor += VERSE_TEX2D(maskSampler, uv.st + vec2(off, off));
    maskColor += VERSE_TEX2D(maskSampler, uv.st + vec2(-off, off));
    maskColor *= 1.0 / 9.0;

    vec3 WSD = worldSunDir, WCP = worldCameraPos;
    vec3 P = vertexInWorld, N = normalize(P);// normalInWorld
    P = N * (length(P) * 0.99);  // FIXME

    float aspect = maskColor.x * radians(360.0), slope = maskColor.y * radians(90.0);
    vec3 localN = vec3(sin(slope) * sin(aspect), sin(slope) * cos(aspect), cos(slope));
    vec3 east = normalize(cross(vec3(0, 1, 0), N));
    vec3 north = normalize(cross(N, east));

    float terrainDetails = 1.0;
    N = mix(N, mat3(east, north, N) * localN, terrainDetails);
    vec3 originalGroundColor = groundColor.rgb;

    float cTheta = max(dot(N, WSD), 0.0); vec3 sunL, skyE;
    sunRadianceAndSkyIrradiance(P, N, WSD, sunL, skyE);
    groundColor.rgb *= max((sunL * cTheta + skyE) / 3.14159265, vec3(0.1));
    groundColor.a *= clamp(globalOpaque, 0.0, 1.0);
    
    vec3 extinction = vec3(1.0);
    vec3 inscatter = inScattering(WCP, P, WSD, extinction, 0.0);
    vec3 compositeColor = groundColor.rgb * extinction * sunColorScale + inscatter * skyColorScale;
    //vec4 finalColor = vec4(hdr(compositeColor), groundColor.a);
    vec4 finalColor = vec4(mix(hdr(compositeColor), originalGroundColor, cTheta), groundColor.a);

    // Color grading work
    finalColor.rgb = colorBalanceFunc(finalColor.rgb, ColorBalance.x, ColorBalance.y, ColorBalance.z, ColorBalanceMode);
    finalColor.rgb = colorAdjustmentFunc(finalColor.rgb, ColorAttribute.x, ColorAttribute.y, ColorAttribute.z);

#ifdef VERSE_GLES3
    fragColor/*Atmospheric Color*/ = finalColor;
    fragOrigin/*Mask Color*/ = maskValue;
#else
    gl_FragData[0]/*Atmospheric Color*/ = finalColor;
    gl_FragData[1]/*Mask Color*/ = maskValue;
#endif
}
