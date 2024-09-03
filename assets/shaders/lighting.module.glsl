#if VERSE_WEBGL1
#extension GL_OES_standard_derivatives : enable
#elif VERSE_WEBGL2
#extension GL_OES_standard_derivatives : enable
#endif

#ifndef PI
#define PI 3.141592653589793
#endif

#ifndef HALF_PI
#define HALF_PI 1.5707963267948966
#endif

float beckmannDistribution(float x, float roughness)
{
    float NdotH = max(x, 0.0001);
    float cos2Alpha = NdotH * NdotH;
    float tan2Alpha = (cos2Alpha - 1.0) / cos2Alpha;
    float roughness2 = roughness * roughness;
    float denom = PI * roughness2 * cos2Alpha * cos2Alpha;
    return exp(tan2Alpha / roughness2) / denom;
}

float signedDistanceSquare(vec2 point, float width)
{
	vec2 d = abs(point) - width;
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

////////////////// LIGHTING
float lambertDiffuse(vec3 lightDirection, vec3 surfaceNormal)
{
    return max(0.0, dot(lightDirection, surfaceNormal));
}

float orenNayarDiffuse(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                       float roughness, float albedo)
{
    float LdotV = dot(lightDirection, viewDirection);
    float NdotL = dot(lightDirection, surfaceNormal);
    float NdotV = dot(surfaceNormal, viewDirection);
    float s = LdotV - NdotL * NdotV;
    float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));
    float sigma2 = roughness * roughness;
    float A = 1.0 + sigma2 * (albedo / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);
    return albedo * max(0.0, NdotL) * (A + B * s / t) / PI;
}

float phongSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess)
{
    vec3 R = -reflect(lightDirection, surfaceNormal);
    return pow(max(0.0, dot(viewDirection, R)), shininess);
}

float blinnPhongSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess)
{
    vec3 H = normalize(viewDirection + lightDirection);
    return pow(max(0.0, dot(surfaceNormal, H)), shininess);
}

float beckmannSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float roughness)
{
    return beckmannDistribution(dot(surfaceNormal, normalize(lightDirection + viewDirection)), roughness);
}

float gaussianSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess)
{
    vec3 H = normalize(lightDirection + viewDirection);
    float theta = acos(dot(H, surfaceNormal));
    float w = theta / shininess; return exp(-w*w);
}

float cookTorranceSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                           float roughness, float fresnel)
{
    float VdotN = max(dot(viewDirection, surfaceNormal), 0.0);
    float LdotN = max(dot(lightDirection, surfaceNormal), 0.0);
    vec3 H = normalize(lightDirection + viewDirection);
    float NdotH = max(dot(surfaceNormal, H), 0.0);
    float VdotH = max(dot(viewDirection, H), 0.000001);
    float x = 2.0 * NdotH / VdotH;
    float G = min(1.0, min(x * VdotN, x * LdotN));
    float D = beckmannDistribution(NdotH, roughness);
    float F = pow(1.0 - VdotN, fresnel);  // Fresnel term
    return G * F * D / max(PI * VdotN * LdotN, 0.000001);
}

float wardSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                   vec3 fiberParallel, vec3 fiberPerpendicular,
                   float shinyParallel, float shinyPerpendicular)
{
    float NdotL = dot(surfaceNormal, lightDirection);
    float NdotR = dot(surfaceNormal, viewDirection);
    if(NdotL < 0.0 || NdotR < 0.0) return 0.0;

    vec3 H = normalize(lightDirection + viewDirection);
    float NdotH = dot(surfaceNormal, H);
    float XdotH = dot(fiberParallel, H);
    float YdotH = dot(fiberPerpendicular, H);
    float coeff = sqrt(NdotL/NdotR) / (4.0 * PI * shinyParallel * shinyPerpendicular); 
    float theta = (pow(XdotH/shinyParallel, 2.0) + pow(YdotH/shinyPerpendicular, 2.0)) / (1.0 + NdotH);
    return coeff * exp(-2.0 * theta);
}

////////////////// EFFECTS
vec2 parallaxOcclusionMapping(sampler2D depthMap, vec2 uv, vec2 displacement, float pivot, int layers)
{
	const float layerDepth = 1.0 / float(layers);
	float currentLayerDepth = 0.0;
	vec2 deltaUv = displacement / float(layers);
	vec2 currentUv = uv + pivot * displacement;
	float currentDepth = texture2D(depthMap, currentUv).r;
	for(int i = 0; i < layers; i++)
    {
		if (currentLayerDepth > currentDepth) break;
		currentUv -= deltaUv;
		currentDepth = texture2D(depthMap, currentUv).r;
		currentLayerDepth += layerDepth;
	}

	vec2 prevUv = currentUv + deltaUv;
	float endDepth = currentDepth - currentLayerDepth;
	float startDepth = texture2D(depthMap, prevUv).r - currentLayerDepth + layerDepth;
	float w = endDepth / (endDepth - startDepth);
	return mix(currentUv, prevUv, w);
}

float vignetteEffect(vec2 uv, vec2 size, float roundness, float smoothness)
{
	uv -= 0.5;  // Center UVs
	float minWidth = min(size.x, size.y);  // Shift UVs based on the larger of width/height
	uv.x = sign(uv.x) * clamp(abs(uv.x) - abs(minWidth - size.x), 0.0, 1.0);
	uv.y = sign(uv.y) * clamp(abs(uv.y) - abs(minWidth - size.y), 0.0, 1.0);
	float boxSize = minWidth * (1.0 - roundness);
	float dist = signedDistanceSquare(uv, boxSize) - (minWidth * roundness);
	return 1.0 - smoothstep(0.0, smoothness, dist);
}
