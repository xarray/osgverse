uniform sampler2D sceneSampler, maskSampler, extraLayerSampler;
uniform sampler2D transmittanceSampler;
uniform sampler2D skyIrradianceSampler;
uniform sampler3D inscatterSampler;
uniform sampler2D glareSampler;
uniform vec3 worldCameraPos, worldSunDir, origin;
uniform vec3 sunColorScale, skyColorScale;
uniform float hdrExposure, globalOpaque;

uniform vec4 clipPlane0, clipPlane1, clipPlane2;

uniform vec3 ColorAttribute;     // (Brightness, Saturation, Contrast)
uniform vec3 ColorBalance;       // (Cyan-Red, Magenta-Green, Yellow-Blue)
uniform int ColorBalanceMode;    // 0 - Shadow, 1 - Midtone, 2 - Highlight

VERSE_FS_IN vec3 normalInWorld;
VERSE_FS_IN vec3 vertexInWorld;
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

vec4 linePlaneIntersection(vec3 linePoint, vec3 lineDir, vec4 plane)
{
    float denominator = dot(plane.xyz, lineDir);
    if (abs(denominator) < 1e-6)
    {
        float distance = dot(plane.xyz, linePoint) + plane.w;
        if (abs(distance) < 1e-6) return vec4(linePoint, 1.0);
        else return vec4(0.0, 0.0, 0.0, 0.0);
    }

    float t = -(dot(plane.xyz, linePoint) + plane.w) / denominator;
    vec3 intersection = linePoint + t * lineDir;
    return vec4(intersection, t);
}

void main()
{
    vec4 worldPos = vec4(vertexInWorld, 1.0);
    float clipD0 = dot(worldPos, clipPlane0), clipD1 = dot(worldPos, clipPlane1), clipD2 = dot(worldPos, clipPlane2);
    if (clipD0 > 0.0 && clipD1 > 0.0 && clipD2 > 0.0)
    {   // Just for test inner rendering...
        vec4 innerColor = vec4(0.0);
        vec3 dir = normalize(vertexInWorld - worldCameraPos);
        vec4 ip0 = linePlaneIntersection(vertexInWorld, dir, clipPlane0);
        vec4 ip1 = linePlaneIntersection(vertexInWorld, dir, clipPlane1);
        vec4 ip2 = linePlaneIntersection(vertexInWorld, dir, clipPlane2);
        if (ip0.w > 0.0 && ip1.w > 0.0 && ip2.w > 0.0)
        {
            if (length(ip0.xyz - ip1.xyz) < 1.0) innerColor = vec4(1.0, 1.0, 1.0, 0.5);
            else if (length(ip0.xyz - ip2.xyz) < 1.0) innerColor = vec4(1.0, 1.0, 1.0, 0.5);
            else if (length(ip1.xyz - ip2.xyz) < 1.0) innerColor = vec4(1.0, 1.0, 1.0, 0.5);
            else if (ip0.w < ip1.w && ip0.w < ip2.w) innerColor = vec4(1.0, 0.0, 0.0, 0.5);
            else if (ip1.w < ip0.w && ip1.w < ip2.w) innerColor = vec4(0.0, 1.0, 0.0, 0.5);
            else innerColor = vec4(0.0, 0.0, 1.0, 0.5);
        }
        else if (ip0.w > 0.0 && ip1.w > 0.0)
        {
            if (length(ip0.xyz - ip1.xyz) < 1.0) innerColor = vec4(1.0, 1.0, 1.0, 0.5);
            else innerColor = (ip0.w < ip1.w) ? vec4(1.0, 0.0, 0.0, 0.5) : vec4(0.0, 1.0, 0.0, 0.5);
        }
        else if (ip0.w > 0.0 && ip2.w > 0.0)
        {
            if (length(ip0.xyz - ip2.xyz) < 1.0) innerColor = vec4(1.0, 1.0, 1.0, 0.5);
            else innerColor = (ip0.w < ip2.w) ? vec4(1.0, 0.0, 0.0, 0.5) : vec4(0.0, 0.0, 1.0, 0.5);
        }
        else if (ip1.w > 0.0 && ip2.w > 0.0)
        {
            if (length(ip1.xyz - ip2.xyz) < 1.0) innerColor = vec4(1.0, 1.0, 1.0, 0.5);
            else innerColor = (ip1.w < ip2.w) ? vec4(0.0, 1.0, 0.0, 0.5) : vec4(0.0, 0.0, 1.0, 0.5);
        }
        else if (ip0.w > 0.0) innerColor = vec4(1.0, 0.0, 0.0, 0.5);
        else if (ip1.w > 0.0) innerColor = vec4(0.0, 1.0, 0.0, 0.5);
        else if (ip2.w > 0.0) innerColor = vec4(0.0, 0.0, 1.0, 0.5);

#ifdef VERSE_GLES3
        fragColor/*Atmospheric Color*/ = innerColor;
        fragOrigin/*Mask Color*/ = vec4(1.0);
#else
        gl_FragData[0]/*Atmospheric Color*/ = innerColor;
        gl_FragData[1]/*Mask Color*/ = vec4(1.0);
#endif
        return;
    }
    
    vec4 groundColor = VERSE_TEX2D(sceneSampler, texCoord.st);
    vec4 layerColor = VERSE_TEX2D(extraLayerSampler, texCoord.st);
    groundColor.rgb = mix(groundColor.rgb, layerColor.rgb, layerColor.a);

    // Mask color: r = aspect, g = slope, b = mask (0 - 0.5: land, 0.5 - 1: ocean)
    vec4 maskColor = VERSE_TEX2D(maskSampler, texCoord.st);
    vec4 maskValue = maskColor.zzza; float off = 0.002;
    maskColor += VERSE_TEX2D(maskSampler, texCoord.st + vec2(-off, 0.0));
    maskColor += VERSE_TEX2D(maskSampler, texCoord.st + vec2(off, 0.0));
    maskColor += VERSE_TEX2D(maskSampler, texCoord.st + vec2(0.0, -off));
    maskColor += VERSE_TEX2D(maskSampler, texCoord.st + vec2(0.0, off));
    maskColor += VERSE_TEX2D(maskSampler, texCoord.st + vec2(-off, -off));
    maskColor += VERSE_TEX2D(maskSampler, texCoord.st + vec2(off, -off));
    maskColor += VERSE_TEX2D(maskSampler, texCoord.st + vec2(off, off));
    maskColor += VERSE_TEX2D(maskSampler, texCoord.st + vec2(-off, off));
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

    float cTheta = dot(N, WSD); vec3 sunL, skyE;
    sunRadianceAndSkyIrradiance(P, N, WSD, sunL, skyE);
    groundColor.rgb *= max((sunL * max(cTheta, 0.0) + skyE) / 3.14159265, vec3(0.1));
    groundColor.a *= clamp(globalOpaque, 0.0, 1.0);
    
    vec3 extinction = vec3(1.0);
    vec3 inscatter = inScattering(WCP, P, WSD, extinction, 0.0);
    vec3 compositeColor = groundColor.rgb * extinction * sunColorScale + inscatter * skyColorScale;
    vec4 finalColor = vec4(hdr(compositeColor), groundColor.a);

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
