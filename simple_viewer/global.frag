#version 130
uniform sampler2D DiffuseMap, LightMap, NormalMap;
uniform sampler2D SpecularMap, ReflectionMap;
uniform mat4 ProjectionToWorld, osg_ViewMatrix;
uniform vec4 dLightDirection, dLightColor;
uniform vec3 metallicRoughness;
in vec4 eyeVertex, texCoord0, texCoord1;
in vec3 eyeNormal, eyeTangent, eyeBinormal;

const vec2 INV_ATAN = vec2(0.1591, 0.3183);
const float GAMMA = 2.2, INV_GAMMA = 1.0 / 2.2;
const float M_PI = 3.14159265;

vec2 sampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= INV_ATAN; uv += 0.5; uv.y = 1.0 - uv.y; return uv;
}

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
    vec3 specularR = specular * numerator / max(denominator, 0.0001);
    
    vec3 radiance = (kD * (albedo / M_PI) + specularR) * radianceIn * nDotL;
    return radiance * shadow;
}

void main()
{
    // Read basic data
    vec2 uv0 = texCoord0.xy, uv1 = texCoord1.xy;
    vec4 color = texture(DiffuseMap, uv0) * gl_Color;
    vec4 shadow = texture(LightMap, uv1);
    vec4 normalValue = texture(NormalMap, uv0);
    vec4 specColor = texture(SpecularMap, uv0);
    float shadowLength = length(shadow.rgb);
    float metallic = metallicRoughness.x, roughness = metallicRoughness.y;
    color.rgb = pow(color.rgb, vec3(GAMMA));
    
    // Compute eye-space normal
    vec3 eyeNormal2 = eyeNormal;
    if (normalValue.a > 0.1)
    {
        vec3 tsNormal = normalize(2.0 * normalValue.rgb - vec3(1.0));
        eyeNormal2 = normalize(mat3(eyeTangent, eyeBinormal, eyeNormal) * tsNormal);
    }
    
    // Rebuild world vertex attributes
    vec4 vecInProj = vec4(uv0.x * 2.0 - 1.0, uv0.y * 2.0 - 1.0, gl_FragCoord.z, 1.0);
    vec4 worldVertex = ProjectionToWorld * vecInProj;
    vec3 worldPos = worldVertex.xyz / worldVertex.w;
    
    // Components common to all light types
    vec3 viewDir = -normalize(osg_ViewMatrix * worldVertex).xyz;
    vec3 R = reflect(-viewDir, eyeNormal2), lightDir = dLightDirection.xyz;
    vec3 albedo = color.rgb, specular = specColor.rgb;
    float nDotV = max(dot(eyeNormal2, viewDir), 0.0);
    
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 radianceOut = computeDirectionalLight(lightDir, dLightColor.xyz, eyeNormal2, viewDir,
                                               albedo, specular, roughness, metallic, 1.0, F0);
    
    
    // Treat ambient light as IBL or not
    vec3 ambient = vec3(0.025) * albedo;
    if (metallicRoughness.z > 0.1)
    {
        float etaRatio = 0.6;
        float fresnel = 0.1 + 1.2 * pow(min(0.0, 1.0 - nDotV), 2.0);
        vec3 T = refract(-viewDir, eyeNormal2, etaRatio);
        
        vec3 reflectionColor = texture(ReflectionMap, sampleSphericalMap(R)).rgb;
        vec3 refractionColor = texture(ReflectionMap, sampleSphericalMap(T)).rgb;
        ambient = mix(ambient, mix(refractionColor, reflectionColor, fresnel), metallicRoughness.z);
    }
    color.rgb = radianceOut + ambient;
    
    // Fog
    const float LOG2 = 1.442695, fogDensity = 0.0006;
    float z = gl_FragCoord.z / gl_FragCoord.w;
    float fogFactor = exp2(-fogDensity * fogDensity * z * z * LOG2);
    vec4 fogColor = vec4(0.8, 0.8, 0.7f, 1.0);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    
    // Final color grading
    vec4 final = (shadowLength > 0.1) ? (color * shadow) : color;
    final = mix(fogColor, final, fogFactor);
    
    vec3 avg = vec3(0.5,0.5,0.5), luncoeff = vec3 (0.2125, 0.7154, 0.0721);
    float T1 = 1.4, T2 = 1.0;
    
    vec3 brtColor = pow(final.rgb, vec3(INV_GAMMA)) * T2;
    float intensity = dot(brtColor, luncoeff);
    vec3 satColor = vec3(mix(vec3(intensity), brtColor, 1.0));
    final.rgb = vec3(mix(avg, satColor, T1));
    gl_FragColor = vec4(final.rgb, color.a);
}
