#version 130
#define AO_RANDOMTEX_SIZE 4
#define M_PI 3.1415926535897932384626433832795
uniform sampler2D NormalBuffer, DepthBuffer, RandomTexture;
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
in vec4 texCoord0;

const float NUM_STEPS = 4;
const float NUM_DIRECTIONS = 8;
float negInvR2 = -1.0 / (1.0 * 1.0);
float nDotVBias = 0.5;
vec2 invFullResolution = vec2(1.0 / 1920.0, 1.0 / 1080.0);
float radiusToScreen = 0.5;
float powExponent = 1.5;
float AOMultiplier = 2.0;

float falloff(float distanceSquare)
{
    // 1 scalar mad instruction
    return distanceSquare * negInvR2 + 1.0;
}

vec4 getJitter()
{
    // Get the current jitter vector
    return texture(RandomTexture, (gl_FragCoord.xy / AO_RANDOMTEX_SIZE));
}

vec2 rotateDirection(vec2 dir, vec2 cosSin)
{
    return vec2(dir.x * cosSin.x - dir.y * cosSin.y,
                dir.x * cosSin.y + dir.y * cosSin.x);
}

vec3 fetchViewPos(vec2 uv)
{
    float depthValue = texture(DepthBuffer, uv).r * 2.0 - 1.0;
    vec4 vecInProj = vec4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, depthValue, 1.0);
    vec4 eyeVertex = GBufferMatrices[3] * vecInProj;
    return eyeVertex.xyz / eyeVertex.w;
}

float computeAO(vec3 P, vec3 N, vec3 S)
{
    // P = view-space position at the kernel center
    // N = view-space normal at the kernel center
    // S = view-space position of the current sample
    vec3 V = S - P; float VdotV = dot(V, V);
    float NdotV = dot(N, V) * 1.0/sqrt(VdotV);
    return clamp(NdotV - nDotVBias, 0, 1) * clamp(falloff(VdotV),0,1);
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
            vec2 snappedUV = round(rayPixels * direction) * invFullResolution + fullResUV;
            vec3 S = fetchViewPos(snappedUV); rayPixels += stepSizePixels;
            AO += computeAO(viewPosition, viewNormal, S);
        }
    }
    AO *= AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);
    return clamp(1.0 - AO * 2.0, 0, 1);
}

void main()
{
	vec2 uv0 = texCoord0.xy;
	vec4 normalAlpha = texture(NormalBuffer, uv0);
    float depthValue = texture(DepthBuffer, uv0).r * 2.0 - 1.0;
    
    // Rebuild world vertex attributes
    vec4 vecInProj = vec4(uv0.x * 2.0 - 1.0, uv0.y * 2.0 - 1.0, depthValue, 1.0);
    vec3 eyePosition = fetchViewPos(uv0), eyeNormal = normalAlpha.rgb;
    
    // Get jitter vector for the current full-res pixel
    float AO = computeCoarseAO(uv0, radiusToScreen, getJitter(), eyePosition, eyeNormal);
    gl_FragColor = vec4(pow(AO, powExponent));
}
