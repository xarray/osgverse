#version 130
#define M_PI 3.1415926535897932384626433832795
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

uniform sampler2D NormalBuffer, DepthBuffer, DiffuseMetallicBuffer;
uniform sampler2D SpecularRoughnessBuffer, EmissionOcclusionBuffer;
uniform sampler2DArray ShadowMapArray;
uniform mat4 LightMatrices[4];
uniform mat4 GBufferProjectionToWorld;
uniform vec3 GBufferCameraPosition;
uniform vec2 NearFarPlanes;
in vec4 texCoord0;

/// PBR functions
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    float val = 1.0 - cosTheta;
    return F0 + (1.0 - F0) * (val*val*val*val*val); //Faster than pow
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    float val = 1.0 - cosTheta;
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * (val*val*val*val*val); //Faster than pow
}

float distributionGGX(vec3 N, vec3 H, float rough)
{
    float a  = rough * rough;
    float a2 = a * a;
    float nDotH  = max(dot(N, H), 0.0);
    float nDotH2 = nDotH * nDotH;
    float num = a2; 
    float denom = (nDotH2 * (a2 - 1.0) + 1.0);
    denom = 1 / (M_PI * denom * denom);
    return num * denom;
}

float geometrySchlickGGX(float nDotV, float rough)
{
    float r = (rough + 1.0);
    float k = r*r / 8.0;
    float num = nDotV;
    float denom = 1 / (nDotV * (1.0 - k) + k);
    return num * denom;
}

float geometrySmith(float nDotV, float nDotL, float rough)
{
    float ggx2 = geometrySchlickGGX(nDotV, rough);
    float ggx1 = geometrySchlickGGX(nDotL, rough);
    return ggx1 * ggx2;
}

/// Lighting functions
float linearDepth(float depthRange/*[-1, 1]*/)
{
    return (2.0 * NearFarPlanes.x * NearFarPlanes.y) /
           ((NearFarPlanes.y + NearFarPlanes.x) - depthRange * (NearFarPlanes.y - NearFarPlanes.x));
}

vec3 computeDirectionalLight(vec3 lightDirection, vec3 lightColor, vec3 normal, vec3 viewDir,
                             vec3 albedo, vec3 specular, float rough, float metal, float shadow, vec3 F0)
{
    // Variables common to BRDFs
    vec3 radianceIn = lightColor, lightDir = normalize(-lightDirection);
    vec3 halfway = normalize(lightDir + viewDir);
    float nDotV = max(dot(normal, viewDir), 0.0);
    float nDotL = max(dot(normal, lightDir), 0.0);

    // Cook-Torrance BRDF
    float NDF = distributionGGX(normal, halfway, rough);
    float G = geometrySmith(nDotV, nDotL, rough);
    vec3 F = fresnelSchlick(max(dot(halfway, viewDir), 0.0), F0);

    // Finding specular and diffuse component
    vec3 kD = (vec3(1.0) - F) * (1.0 - metal);
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * nDotV * nDotL;
    vec3 specularR = numerator / max(denominator, 0.0001) + specular;
    
    vec3 radiance = (kD * (albedo / M_PI) + specularR) * radianceIn * nDotL;
    return radiance * shadow;
}

vec3 computePointLight(vec3 lightPosition, vec3 lightColor, float range, vec3 normal, vec3 fragPos,
                       vec3 viewDir, vec3 albedo, vec3 specular, float rough, float metal, vec3 F0)
{
    vec3 position = lightPosition, color = lightColor;
    float radius = range;

    // Stuff common to the BRDF subfunctions 
    vec3 lightDir = normalize(position - fragPos);
    vec3 halfway = normalize(lightDir + viewDir);
    float nDotV = max(dot(normal, viewDir), 0.0);
    float nDotL = max(dot(normal, lightDir), 0.0);

    // Attenuation calculation that is applied to all
    float distance = length(position - fragPos);
    float attenuation = pow(clamp(1.0 - pow((distance / radius), 4.0), 0.0, 1.0), 2.0)
                      / (1.0  + (distance * distance));
    vec3 radianceIn = color * attenuation;

    // Cook-Torrance BRDF
    float NDF = distributionGGX(normal, halfway, rough);
    float G = geometrySmith(nDotV, nDotL, rough);
    vec3 F = fresnelSchlick(max(dot(halfway, viewDir), 0.0), F0);

    // Finding specular and diffuse component
    vec3 kD = (vec3(1.0) - F) * (1.0 - metal);
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * nDotV * nDotL;
    vec3 specularR = numerator / max(denominator, 0.0001) + specular;

    vec3 radiance = (kD * (albedo / M_PI) + specularR) * radianceIn * nDotL;
    return radiance;//radiance * (1.0 - shadow);  // FIXME: point shadow
}

/// Main entry
void main()
{
	vec2 uv0 = texCoord0.xy;
	vec4 diffuseMetallic = texture(DiffuseMetallicBuffer, uv0);
	vec4 specularRoughness = texture(SpecularRoughnessBuffer, uv0);
	vec4 emissionOcclusion = texture(EmissionOcclusionBuffer, uv0);
    vec4 normalAlpha = texture(NormalBuffer, uv0);
    float depthValue = texture(DepthBuffer, uv0).r * 2.0 - 1.0;
    float linearDepth = linearDepth(depthValue), alpha = normalAlpha.a;
    if (alpha < 0.1) discard;
    
    // Rebuild world vertex attributes
    vec4 vecInProj = vec4(uv0.x * 2.0 - 1.0, uv0.y * 2.0 - 1.0, depthValue, 1.0);
    vec4 worldVertex = GBufferProjectionToWorld * vecInProj;
    vec3 eyeNormal = normalAlpha.rgb, worldPos = worldVertex.xyz / worldVertex.w;
    
    vec3 dLightDir = vec3(0.0, 0.0, -1.0), dLightColor = vec3(5.0, 5.0, 5.0);  // TODO!!!!!!!!!!!!!
    
    // Components common to all light types
    vec3 viewDir = normalize(GBufferCameraPosition - worldPos);
    vec3 R = reflect(-viewDir, eyeNormal);
    vec3 albedo = diffuseMetallic.rgb, specular = specularRoughness.rgb, emission = emissionOcclusion.rgb;
    float metallic = diffuseMetallic.a, roughness = specularRoughness.a, ao = emissionOcclusion.a;
    float nDotV = max(dot(eyeNormal, viewDir), 0.0), shadow = 1.0;  // TODO!!!!!!!!!!!!! ao
    
    // Compute direcional light and shadow
    /*float shadowLayer = 0.0;  // TODO!!!!!!!!!!!!! shadowLayer
    vec4 lightProjVec = LightMatrices[int(shadowLayer)] * worldVertex;
    vec2 lightProjUV = (lightProjVec.xy / lightProjVec.w) * 0.5 + vec2(0.5);
    {
        vec4 lightProjVec0 = texture(ShadowMapArray, vec3(lightProjUV.xy, shadowLayer));
        float depth = lightProjVec.z / lightProjVec.w, depth0 = lightProjVec0.z + 0.005;
        shadow *= (lightProjVec0.x > 0.1 && depth > depth0) ? 0.5 : 1.0;
    }
    */
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 radianceOut = computeDirectionalLight(dLightDir, dLightColor, eyeNormal, viewDir,
                                               albedo, specular, roughness, metallic, shadow, F0);
    
    // Treat ambient light as IBL or not
    vec3 ambient = vec3(0.025) * albedo;
    if (false)
    {
        vec3 kS = fresnelSchlickRoughness(nDotV, F0, roughness);
        vec3 kD = (1.0 - kS) * (1.0 - metallic);
        vec3 irradiance = texture(irradianceMap, eyeNormal).rgb;
        vec3 diffuse = irradiance * albedo;

        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 envBRDF = texture(brdfLUT, vec2(nDotV, roughness)).rg;
        vec3 envSpecular = prefilteredColor * (kS * envBRDF.x + envBRDF.y);
        ambient = kD * diffuse + envSpecular;
    }
    
    radianceOut += ambient + emission;
	gl_FragColor = vec4(radianceOut, 1.0);
    if (gl_FragCoord.x < 960) gl_FragColor = vec4(albedo, 1.0);  // FIXNE: test only
}
