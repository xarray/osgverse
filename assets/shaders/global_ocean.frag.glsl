#extension GL_EXT_gpu_shader4 : enable
uniform sampler2D sceneSampler;
uniform sampler2D glareSampler;
uniform sampler2D transmittanceSampler;
uniform sampler2D skyIrradianceSampler;
uniform sampler3D inscatterSampler;

uniform vec3 worldCameraPos;
uniform vec3 worldSunDir;
uniform vec2 screenSize;
uniform float hdrExposure;

uniform mat4 cameraToOcean, oceanToCamera, oceanToWorld;
uniform mat4 screenToCamera, cameraToScreen;
uniform vec3 oceanCameraPos, oceanSunDir;
uniform vec3 horizon1, horizon2;
uniform float radius;

uniform sampler2D earthMaskSampler;  // original earth scene
uniform sampler1D wavesSampler; // waves parameters (h, omega, kx, ky) in wind space
uniform float nbWaves; // number of waves
uniform float heightOffset; // so that surface height is centered around z = 0
uniform float seaRoughness; // total variance
uniform float time, oceanOpaque;
uniform vec3 seaColor; // sea bottom color
uniform vec4 lods;  // grid cell size in pixels, angle under which a grid cell is seen,
                    // and parameters of the geometric series used for wavelengths

#define SUN_INTENSITY 100.0
#define PLANET_RADIUS 6360000.0
#define NYQUIST_MIN 0.5 // wavelengths below NYQUIST_MIN * sampling period are fully attenuated
#define NYQUIST_MAX 1.25 // wavelengths above NYQUIST_MAX * sampling period are not attenuated at all
const float PI = 3.141592657;
const float g = 9.81;

VERSE_FS_IN vec3 oceanUv; // coordinates in wind space used to compute P(u)
VERSE_FS_IN vec3 oceanP; // wave point P(u) in ocean space
VERSE_FS_IN vec3 oceanDPdu; // dPdu in wind space, used to compute N
VERSE_FS_IN vec3 oceanDPdv; // dPdv in wind space, used to compute N
VERSE_FS_IN float oceanSigmaSq; // variance of unresolved waves in wind space
VERSE_FS_IN float oceanLod;
VERSE_FS_OUT vec4 fragData;

#include "scattering.module.glsl"
vec3 sunRadiance(float r, float muS);
vec3 skyIrradiance(float r, float muS);
vec3 inScattering(vec3 camera, vec3 point, vec3 sundir, out vec3 extinction, float shaftWidth);

vec3 getWorldCameraPos() { return worldCameraPos; }
vec3 getWorldSunDir() { return worldSunDir; }
float getHdrExposure() { return hdrExposure; }

vec3 hdr(vec3 L)
{
    L = L * hdrExposure;
    L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
    L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
    L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
    return L;
}

void sunRadianceAndSkyIrradiance2(vec3 worldP, vec3 worldN, vec3 worldS, out vec3 sunL, out vec3 skyE)
{
    float r = length(worldP);
    if (r < 0.9 * PLANET_RADIUS)
    {
        worldP.z += PLANET_RADIUS;
        r = length(worldP);
    }

    vec3 worldV = worldP / r; // vertical vector
    float muS = dot(worldV, worldS);
    sunL = sunRadiance(r, muS);
    skyE = skyIrradiance(r, muS) * (1.0 + dot(worldV, worldN));
}

float erfc(float x)
{ return 2.0 * exp(-x * x) / (2.319 * x + sqrt(4.0 + 1.52 * x * x)); }

float Lambda(float cosTheta, float sigmaSq)
{
    float v = cosTheta / sqrt((1.0 - cosTheta * cosTheta) * (2.0 * sigmaSq));
    return max(0.0, (exp(-v * v) - v * sqrt(PI) * erfc(v)) / (2.0 * v * sqrt(PI)));
}

// L, V, N in world space
float reflectedSunRadiance(vec3 L, vec3 V, vec3 N, float sigmaSq)
{
    vec3 H = normalize(L + V);
    vec3 Ty = normalize(cross(N, vec3(1.0, 0.0, 0.0)));
    vec3 Tx = cross(Ty, N);
    float zetax = dot(H, Tx) / dot(H, N);
    float zetay = dot(H, Ty) / dot(H, N);

    float zL = dot(L, N); // cos of source zenith angle
    float zV = dot(V, N); // cos of receiver zenith angle
    float zH = dot(H, N); // cos of facet normal zenith angle
    float zH2 = zH * zH;
    float p = exp(-0.5 * (zetax * zetax + zetay * zetay) / sigmaSq) / (2.0 * PI * sigmaSq);
    float fresnel = 0.02 + 0.98 * pow(1.0 - dot(V, H), 5.0);

    zL = max(zL, 0.01); zV = max(zV, 0.01);
    return fresnel * p / ((1.0 + Lambda(zL, sigmaSq) + Lambda(zV, sigmaSq)) * zV * zH2 * zH2 * 4.0);
}

// L, V, N in world space
float wardReflectedSunRadiance(vec3 L, vec3 V, vec3 N, float sigmaSq)
{
    vec3 H = normalize(L + V);
    float hn = dot(H, N);
    float p = exp(-2.0 * ((1.0 - hn * hn) / sigmaSq) / (1.0 + hn)) / (4.0 * PI * sigmaSq);

    float c = 1.0 - dot(V, H);
    float c2 = c * c;
    float fresnel = 0.02 + 0.98 * c2 * c2 * c;
    float zL = dot(L, N);
    float zV = dot(V, N);
    zL = max(zL, 0.01);
    zV = max(zV, 0.01);

    // brdf times cos(thetaL)
    return zL <= 0.0 ? 0.0 : max(fresnel * p * sqrt(abs(zL / zV)), 0.0);
}

float meanFresnel(float cosThetaV, float sigmaV)
{ return pow(1.0 - cosThetaV, 5.0 * exp(-2.69 * sigmaV)) / (1.0 + 22.7 * pow(sigmaV, 1.5)); }

float meanFresnel(vec3 V, vec3 N, float sigmaSq)
{ return meanFresnel(dot(V, N), sqrt(sigmaSq)); }

float refractedSeaRadiance(vec3 V, vec3 N, float sigmaSq)
{ return 0.98 * (1.0 - meanFresnel(V, N, sigmaSq)); }

void main()
{
    vec2 quadUV = vec2(gl_FragCoord.x / screenSize.x, gl_FragCoord.y / screenSize.y);
    vec4 sceneColor = VERSE_TEX2D(earthMaskSampler, quadUV);
    if (oceanUv.z < 0.5 || sceneColor.a < 0.1) discard;

    vec3 WSD = getWorldSunDir(), WCP = getWorldCameraPos();
    vec3 dPdu = oceanDPdu, dPdv = oceanDPdv; vec2 uv = oceanUv.xy;
    float lod = oceanLod, sigmaSq = oceanSigmaSq;

    float iMAX = min(ceil((log2(NYQUIST_MAX * lod) - lods.z) * lods.w), nbWaves - 1.0);
    float iMax = floor((log2(NYQUIST_MIN * lod) - lods.z) * lods.w);
    float iMin = max(0.0, floor((log2(NYQUIST_MIN * lod / lods.x) - lods.z) * lods.w));
    for (float i = iMin; i <= iMAX; i += 1.0)
    {
        vec4 wt = VERSE_TEX1D(wavesSampler, (i + 0.5) / nbWaves);
        //vec4 wt = textureLod(wavesSampler, (i + 0.5) / nbWaves, 0.0);
        float phase = wt.y * time - dot(wt.zw, uv);
        float s = sin(phase), c = cos(phase);
        float overk = g / (wt.y * wt.y);

        float wm = smoothstep(NYQUIST_MIN, NYQUIST_MAX, (2.0 * PI) * overk / lod);
        float wn = smoothstep(NYQUIST_MIN, NYQUIST_MAX, (2.0 * PI) * overk / lod * lods.x);
        vec3 factor = (1.0 - wm) * wn * wt.x * vec3(wt.zw * overk, 1.0);
        vec3 dPd = factor * vec3(c, c, -s);
        dPdu -= dPd * wt.z; dPdv -= dPd * wt.w;
        wt.zw *= overk;

        float kh = i < iMax ? wt.x / overk : 0.0;
        float wkh = (1.0 - wn) * kh;
        sigmaSq -= wt.z * wt.z * (sqrt(1.0 - wkh * wkh) - sqrt(1.0 - kh * kh));
    }
    
    vec3 earthCamera = vec3(0.0, 0.0, oceanCameraPos.z + radius);
    vec3 earthP = radius > 0.0 ? normalize(oceanP + vec3(0.0, 0.0, radius)) * (radius + 10.0) : oceanP;
    vec3 oceanCamera = vec3(0.0, 0.0, oceanCameraPos.z);
    vec3 V = normalize(oceanCamera - oceanP);
    vec3 N = normalize(cross(dPdu, dPdv));
    if (dot(V, N) < 0.0) N = reflect(N, V); // reflects backfacing normals

    vec3 sunL, skyE, extinction;
    sunRadianceAndSkyIrradiance2(earthP, N, oceanSunDir, sunL, skyE);

    vec3 Lsun = wardReflectedSunRadiance(oceanSunDir, V, N, sigmaSq) * sunL;
    vec3 Lsky = meanFresnel(V, N, sigmaSq) * skyE / PI;
    vec3 Lsea = refractedSeaRadiance(V, N, sigmaSq) * seaColor * skyE / PI;
    vec3 surfaceColor = Lsun + Lsky + Lsea;

    // aerial perspective
    vec3 inscatter = inScattering(earthCamera, earthP, oceanSunDir, extinction, 0.0);
    vec3 finalColor = surfaceColor * extinction + inscatter;
    fragData.rgb = hdr(finalColor); fragData.a = clamp(oceanOpaque, 0.0, 1.0);

    // Input sceneColor should be a black/white image to show where ocean is...
    fragData.a *= 1.0 - length(clamp(sceneColor.r, 0.0, 1.0));
    VERSE_FS_FINAL(fragData);
}
