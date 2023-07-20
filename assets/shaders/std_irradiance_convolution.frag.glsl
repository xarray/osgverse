#define M_PI 3.1415926535897932384626433832795
uniform sampler2D EnvironmentMap;
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

void main()
{
    vec3 N = normalize(invSphericalUV(texCoord0.xy));
    vec3 prefilteredColor = vec3(0.0), irradiance = vec3(0.0), up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, N); up = cross(N, right);  // tangent space calculation
    
    float delta = 0.05, nrSamples = 0.0f, maxPhi = 2.0 * M_PI, maxT = 0.5 * M_PI;
    for (float phi = 0.0; phi < maxPhi; phi += delta)
    {
        for (float theta = 0.0; theta < maxT; theta += delta)
        {
            // Spherical to cartesian (in tangent space), then tangent to world
            float sinT = sin(theta), cosT = cos(theta);
            vec3 tangentVec = vec3(sinT * cos(phi), sinT * sin(phi), cosT);
            vec3 sampleVec = tangentVec.x * right + tangentVec.y * up + tangentVec.z * N;
            irradiance += VERSE_TEX2D(EnvironmentMap, sphericalUV(sampleVec)).rgb * cosT * sinT;
            nrSamples += 1.0f;
        }
    }
    
    irradiance = M_PI * irradiance * (1.0 / float(nrSamples));
    fragData = vec4(irradiance, 1.0);
    VERSE_FS_FINAL(fragData);
}
