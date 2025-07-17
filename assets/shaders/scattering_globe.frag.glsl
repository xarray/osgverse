uniform sampler2D sceneSampler;
uniform sampler2D glareSampler;
uniform sampler2D transmittanceSampler;
uniform sampler2D skyIrradianceSampler;
uniform sampler3D inscatterSampler;
uniform vec3 worldCameraPos;
uniform vec3 worldSunDir;
uniform vec3 origin;
uniform float hdrExposure;

VERSE_FS_IN vec3 normalInWorld;
VERSE_FS_IN vec3 vertexInWorld;
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
    vec3 WSD = worldSunDir, WCP = worldCameraPos;
    vec3 P = vertexInWorld, N = normalInWorld;// normalize(P);
    float cTheta = dot(N, WSD);
    P = N * (length(P) * 0.99);  // FIXME
    
    vec3 sunL, skyE;
    sunRadianceAndSkyIrradiance(P, N, WSD, sunL, skyE);
    
    vec4 groundColor = VERSE_TEX2D(sceneSampler, texCoord.st);
    groundColor.rgb *= max((sunL * max(cTheta, 0.0) + skyE) / 3.14159265, vec3(0.1));
    
    vec3 extinction = vec3(1.0);
    vec3 inscatter = inScattering(WCP, P, WSD, extinction, 0.0);
    vec3 finalColor = groundColor.rgb * extinction + inscatter;
    fragColor = vec4(hdr(finalColor), groundColor.a);
    VERSE_FS_FINAL(fragColor);
}
