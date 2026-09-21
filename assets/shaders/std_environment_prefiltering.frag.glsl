#extension GL_EXT_gpu_shader4: enable
#define M_PI 3.1415926535897932384626433832795
uniform sampler2D EnvironmentMap;
uniform float GlobalRoughness;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 sphericalUV(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan; uv += vec2(0.5); return uv;
}

const vec2 Atan = vec2(6.28318, 3.14159);
vec3 invSphericalUV(vec2 v)
{
    vec2 uv = (v - vec2(0.5)) * Atan; float cosT = cos(uv.y);
    return vec3(cosT * cos(uv.x), sin(uv.y), cosT * sin(uv.x));
}

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float NdotH = max(dot(N, H), 0.0);
    float a2 = a * a, NdotH2 = NdotH * NdotH;
    float nom = a2, denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return nom / (M_PI * denom * denom);
}

// The radiance coming from the environment map is bounded before it is accumulated. An HDR
// panorama may contain pixels close to the maximum exponent of its encoding (the sun disc is
// often much brighter than 1e4, and a saturated RGBE pixel decodes to about 1e38): summing
// hundreds of such samples overflows the 32 bit float of the accumulator, and an infinite
// result is written to the cached IBL image, where every surface reflecting that direction
// turns black. Bounding the source keeps the sun contribution (which is what the specular
// reflection is made of) while making the value of the whole chain finite. The offline baked
// .ibl.rseq files have to be generated again for the fix to show (see
// osgVerse_Test_Pbr_Prerequisite), as the ones already written still hold the old values
const float MAX_ENVIRONMENT_RADIANCE = 10000.0;

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation
float radicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley(uint i, uint N)
{ return vec2(float(i) / float(N), radicalInverse_VdC(i)); }

vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness * roughness, phi = 2.0 * M_PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    // from spherical coordinates to cartesian coordinates - halfway vector
    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    
    // from tangent-space H vector to world-space sample vector
    vec3 up = (abs(N.z) < 0.999) ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

void main()
{
    vec3 N = normalize(invSphericalUV(texCoord0.xy));
    vec3 prefilteredColor = vec3(0.0), R = N, V = N;
    float totalWeight = 0.0;
    
    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        // generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H = importanceSampleGGX(Xi, N, GlobalRoughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        float nDotL = max(dot(N, L), 0.0);
        if (nDotL > 0.0)
        {
            prefilteredColor += min(VERSE_TEX2D(EnvironmentMap, sphericalUV(L)).rgb,
                                    vec3(MAX_ENVIRONMENT_RADIANCE)) * nDotL;
            totalWeight += nDotL;
        }
    }

    // A direction whose samples all point away from the surface would leave the weight at zero,
    // and dividing by it would write a NaN in the IBL image (see the comment of
    // MAX_ENVIRONMENT_RADIANCE above for what such a value does to the rendering)
    fragData = vec4(totalWeight > 0.0 ? prefilteredColor / totalWeight : vec3(0.0), 1.0);
    VERSE_FS_FINAL(fragData);
}
