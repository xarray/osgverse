#version 130
uniform sampler2D NormalBuffer, DepthBuffer;
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
in vec4 texCoord0;

#define NUM_SAMPLES 8
#define NUM_NOISE 4
uniform vec3 ssaoSamples[NUM_SAMPLES];
uniform vec3 ssaoNoises[NUM_NOISE];
float radius = 0.6;
float bias = 0.005;
float magnitude = 1.1;
float contrast = 1.1;

void main()
{
	vec2 uv0 = texCoord0.xy;
	vec4 normalAlpha = texture(NormalBuffer, uv0);
    float depthValue = texture(DepthBuffer, uv0).r * 2.0 - 1.0;
    float occlusion = NUM_SAMPLES;
    
    // Rebuild eye-space vertex attributes
    vec4 vecInProj = vec4(uv0.x * 2.0 - 1.0, uv0.y * 2.0 - 1.0, depthValue, 1.0);
    vec4 eyeVertex = GBufferMatrices[3] * vecInProj;
    vec3 eyeNormal = normalAlpha.rgb, eyePosition = eyeVertex.xyz / eyeVertex.w;
    
    // Compute eye-space TBN
    int noiseS = int(sqrt(NUM_NOISE));
    int noiseX = int(gl_FragCoord.x - 0.5) % noiseS;
    int noiseY = int(gl_FragCoord.y - 0.5) % noiseS;
    vec3 randomVec = ssaoNoises[noiseX + (noiseY * noiseS)];
    vec3 eyeTangent = normalize(randomVec - eyeNormal * dot(randomVec, eyeNormal));
    vec3 eyeBinormal = cross(eyeNormal, eyeTangent);
    mat3 TBN = mat3(eyeTangent, eyeBinormal, eyeNormal);

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec3 samplePosition = TBN * ssaoSamples[i];  // eye-space sample-pos
        samplePosition = eyePosition + samplePosition * radius;

        vec4 offsetUV = GBufferMatrices[2] * vec4(samplePosition, 1.0); offsetUV.xyz /= offsetUV.w;
        depthValue = texture(DepthBuffer, offsetUV.xy * 0.5 + 0.5).r * 2.0 - 1.0;
        eyeVertex = GBufferMatrices[3] * vec4(offsetUV.x, offsetUV.y, depthValue, 1.0);
        
        vec3 offsetPosition = eyeVertex.xyz / eyeVertex.w;
        float occluded = (samplePosition.z - bias > offsetPosition.z) ? 0.0f : 1.0f;
        occluded *= smoothstep(0.0f, 1.0f, radius / abs(eyePosition.z - offsetPosition.z));
        occlusion -= occluded;
    }
    
    occlusion = pow(occlusion / NUM_SAMPLES, magnitude);
    occlusion = contrast * (occlusion - 0.5) + 0.5;
	gl_FragColor = vec4(vec3(occlusion), 1.0);
}
