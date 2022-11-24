#define AO_RANDOMTEX_SIZE 4
#define M_PI 3.1415926535897932384626433832795
uniform sampler2D NormalBuffer, DepthBuffer, RandomTexture;
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
uniform vec2 NearFarPlanes, InvScreenResolution;
uniform float AORadius, AOBias, AOPowExponent;
VERSE_FS_IN vec4 texCoord0;
VERSE_FS_OUT vec4 fragData;

const float NUM_STEPS = 4;
const float NUM_DIRECTIONS = 8;
float projScale = 1.0 / (tan((M_PI * 0.25) * 0.5) * 2.0);
float negInvR2 = -1.0 / (AORadius * AORadius);
float radiusToScreen = AORadius * 0.5 * projScale / InvScreenResolution.y;
float AOMultiplier = 1.0 / (1.0 - AOBias);

float falloff(float distanceSquare)
{
    // 1 scalar mad instruction
    return distanceSquare * negInvR2 + 1.0;
}

vec4 getJitter()
{
    // Get the current jitter vector
    return VERSE_TEX2D(RandomTexture, (gl_FragCoord.xy / AO_RANDOMTEX_SIZE));
}

vec2 rotateDirection(vec2 dir, vec2 cosSin)
{
    return vec2(dir.x * cosSin.x - dir.y * cosSin.y,
                dir.x * cosSin.y + dir.y * cosSin.x);
}

vec3 fetchViewPos(vec2 uv)
{
    mat4 projMatrix = GBufferMatrices[2];
    vec4 projInfo = vec4(2.0f / projMatrix[0][0], 2.0f / projMatrix[1][1],
                         -(1.0f - projMatrix[2][0]) / projMatrix[0][0],
                         -(1.0f + projMatrix[2][1]) / projMatrix[1][1]);
    
    float depthValue = VERSE_TEX2D(DepthBuffer, uv).r;
    float eyeZ = (NearFarPlanes[0] * NearFarPlanes[1])
               / ((NearFarPlanes[0] - NearFarPlanes[1]) * depthValue + NearFarPlanes[1]);
    return vec3(uv * projInfo.xy + projInfo.zw, 1.0) * eyeZ;
}

vec3 minDiff(vec3 P, vec3 Pr, vec3 Pl)
{
    vec3 v1 = Pr - P, v2 = P - Pl;
    return (dot(v1, v1) < dot(v2, v2)) ? v1 : v2;
}

vec3 reconstructNormal(vec2 uv, vec3 P)
{
    vec3 Pr = fetchViewPos(uv + vec2(InvScreenResolution.x, 0));
    vec3 Pl = fetchViewPos(uv + vec2(-InvScreenResolution.x, 0));
    vec3 Pt = fetchViewPos(uv + vec2(0, InvScreenResolution.y));
    vec3 Pb = fetchViewPos(uv + vec2(0, -InvScreenResolution.y));
    return normalize(cross(minDiff(P, Pr, Pl), minDiff(P, Pt, Pb)));
}

float computeAO(vec3 P, vec3 N, vec3 S)
{
    // P = view-space position at the kernel center
    // N = view-space normal at the kernel center
    // S = view-space position of the current sample
    vec3 V = S - P; float VdotV = dot(V, V);
    float NdotV = dot(N, V) * 1.0 / sqrt(VdotV);
    return clamp(NdotV - AOBias, 0.0, 1.0) * clamp(falloff(VdotV), 0.0, 1.0);
}

float computeCoarseAO(vec2 fullResUV, float radiusPixels, vec4 rand, vec3 viewPosition, vec3 viewNormal)
{
    // Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
    float stepSizePixels = radiusPixels / (NUM_STEPS + 1), AO = 0;
    const float alpha = 2.0 * M_PI / NUM_DIRECTIONS;
    for (float directionIndex = 0; directionIndex < NUM_DIRECTIONS; ++directionIndex)
    {
        // Compute normalized 2D direction
        float angle = alpha * directionIndex;
        vec2 direction = rotateDirection(vec2(cos(angle), sin(angle)), rand.xy);

        // Jitter starting sample within the first step
        float rayPixels = (rand.z * stepSizePixels + 1.0);
        for (float stepIndex = 0; stepIndex < NUM_STEPS; ++stepIndex)
        {
            vec2 snappedUV = round(vec2(rayPixels) * direction) * InvScreenResolution + fullResUV;
            vec3 S = fetchViewPos(snappedUV); rayPixels += stepSizePixels;
            AO += computeAO(viewPosition, viewNormal, S);
        }
    }
    AO *= AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);
    return clamp(1.0 - AO * 2.0, 0.0, 1.0);
}

void main()
{
    // Reconstruct view-space normal from nearest neighbors
    vec2 uv0 = texCoord0.xy;
    vec3 eyePosition = fetchViewPos(uv0);
    vec3 eyeNormal = -reconstructNormal(uv0, eyePosition);
    
    // Compute projection of disk of radius into screen space
    float radiusPixels = radiusToScreen / eyePosition.z;
    
    // Get jitter vector for the current full-res pixel
    float AO = computeCoarseAO(uv0, radiusPixels, getJitter(), eyePosition, eyeNormal);
    fragData = vec4(pow(AO, AOPowExponent));
    VERSE_FS_FINAL(fragData);
}
