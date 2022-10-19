#version 130
#define DEBUG_SHADOW_COLOR 0
uniform sampler2D ColorBuffer, IblAmbientBuffer, NormalBuffer, DepthBuffer;
uniform sampler2D ShadowMap0, ShadowMap1, ShadowMap2, ShadowMap3;
uniform mat4 ShadowSpaceMatrices[4];
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
in vec4 texCoord0;
out vec4 fragData;

sampler2D getShadowMap(int id)
{
    return (id < 2) ? ((id == 0) ? ShadowMap0 : ShadowMap1)
                    : ((id == 2) ? ShadowMap2 : ShadowMap3);
}

float getShadowValue(in sampler2D shadowMap, in vec2 lightProjUV, in float depth)
{
    vec4 lightProjVec0 = texture(shadowMap, lightProjUV.xy);
    float depth0 = lightProjVec0.z + 0.001;
    return (lightProjVec0.x > 0.1 && depth > depth0) ? 0.0 : 1.0;
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
        
        float depth = lightProjVec.z / lightProjVec.w;
        float shadowValue = getShadowValue(getShadowMap(i), lightProjUV.xy, depth);
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
