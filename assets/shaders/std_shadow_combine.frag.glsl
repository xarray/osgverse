#version 130
#define DEBUG_SHADOW_COLOR 0
uniform sampler2D ColorBuffer, IblAmbientBuffer, NormalBuffer, DepthBuffer;
uniform sampler2D ShadowMap0, ShadowMap1, ShadowMap2, ShadowMap3;
uniform sampler1D RandomTexture0, RandomTexture1;
uniform mat4 ShadowSpaceMatrices[4];
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
in vec4 texCoord0;
out vec4 fragData;

float getShadowValue(in sampler2D shadowMap, in vec2 lightProjUV, in float depth)
{
    vec4 lightProjVec0 = texture(shadowMap, lightProjUV.xy);
    float depth0 = lightProjVec0.z + 0.005;
    //return (lightProjVec0.x > 0.1 && depth > depth0) ? 0.0 : 1.0;
    return (depth > depth0) ? 0.0 : 1.0;
}

float getShadowPCF_DirectionalLight(in sampler2D shadowMap, in vec2 lightProjUV, in float depth, in float uvRadius)
{
	float sum = 0;
	for (int i = 0; i < 16; i++)
	{
        vec2 dir = texture(RandomTexture1, float(i) / 16.0).xy * 2.0 - vec2(1.0);
		sum += getShadowValue(shadowMap, lightProjUV.xy + dir * uvRadius, depth);
	}
	return sum / 16.0;
}

void main()
{
	vec2 uv0 = texCoord0.xy;
    vec4 colorData = texture(ColorBuffer, uv0), iblData = texture(IblAmbientBuffer, uv0);
    vec4 normalAlpha = texture(NormalBuffer, uv0);
    float depthValue = texture(DepthBuffer, uv0).r * 2.0 - 1.0;
    
    // Rebuild world vertex attributes
    vec4 vecInProj = vec4(uv0.x * 2.0 - 1.0, uv0.y * 2.0 - 1.0, depthValue, 1.0);
    vec4 eyeVertex = GBufferMatrices[3] * vecInProj;
    vec3 eyeNormal = normalAlpha.rgb;
    
    // Compute shadow and combine with color
#if DEBUG_SHADOW_COLOR
    vec3 shadowColors[4], debugShadowColor = vec3(1, 1, 1);
    shadowColors[0] = vec3(1, 0, 0); shadowColors[1] = vec3(0, 1, 0);
    shadowColors[2] = vec3(0, 0, 1); shadowColors[3] = vec3(0, 1, 1);
#endif
    float shadow = 1.0;
    for (int i = 0; i < 4; ++i)
    {
        vec4 lightProjVec = ShadowSpaceMatrices[i] * eyeVertex;
        vec2 lightProjUV = (lightProjVec.xy / lightProjVec.w) * 0.5 + vec2(0.5);
        if (any(lessThan(lightProjUV, vec2(0.0))) || any(greaterThan(lightProjUV, vec2(1.0)))) continue;
        
        float depth = lightProjVec.z / lightProjVec.w;  // real depth in light space
        float shadowValue = 1.0, pcfRadius = 0.0012;
        
        if (i == 0) shadowValue = getShadowPCF_DirectionalLight(ShadowMap0, lightProjUV.xy, depth, pcfRadius);
        else if (i == 1) shadowValue = getShadowPCF_DirectionalLight(ShadowMap1, lightProjUV.xy, depth, pcfRadius);
        else if (i == 2) shadowValue = getShadowPCF_DirectionalLight(ShadowMap2, lightProjUV.xy, depth, pcfRadius);
        else if (i == 3) shadowValue = getShadowPCF_DirectionalLight(ShadowMap3, lightProjUV.xy, depth, pcfRadius);
        shadow *= shadowValue;
#if DEBUG_SHADOW_COLOR
        if (shadowValue < 0.5) debugShadowColor = shadowColors[i];
#endif
    }
    
#if DEBUG_SHADOW_COLOR
    colorData.rgb *= debugShadowColor;
#else
    colorData.rgb *= shadow;
#endif
    colorData.rgb += iblData.rgb;
	fragData = colorData;
}
