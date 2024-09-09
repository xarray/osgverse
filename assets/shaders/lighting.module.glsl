#ifndef PI
#define PI 3.141592653589793
#endif

#ifndef HALF_PI
#define HALF_PI 1.5707963267948966
#endif

float VERSE_beckmannDistribution(float x, float roughness)
{
    float NdotH = max(x, 0.0001);
    float cos2Alpha = NdotH * NdotH;
    float tan2Alpha = (cos2Alpha - 1.0) / cos2Alpha;
    float roughness2 = roughness * roughness;
    float denom = PI * roughness2 * cos2Alpha * cos2Alpha;
    return exp(tan2Alpha / roughness2) / denom;
}

float VERSE_signedDistanceSquare(vec2 point, float width)
{
	vec2 d = abs(point) - width;
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

/* Computes diffuse intensity in Lambertian lighting model
   - lightDirection: unit length vec3 from the surface point toward the light
   - surfaceNormal: unit length normal at the sample point
*/
float VERSE_lambertDiffuse(vec3 lightDirection, vec3 surfaceNormal)
{
    return max(0.0, dot(lightDirection, surfaceNormal));
}

/* Compute diffuse intensity in Oren-Nayar lighting model
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - roughness: measuring the surface roughness, 0 for smooth, 1 for matte
   - albedo: measuring the intensity of the diffuse reflection, >0.96 do not conserve energy
*/
float VERSE_orenNayarDiffuse(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
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

/* Computes specular power in Phong lighting model
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - shininess: exponent in the Phong equation
*/
float VERSE_phongSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess)
{
    vec3 R = -reflect(lightDirection, surfaceNormal);
    return pow(max(0.0, dot(viewDirection, R)), shininess);
}

/* Computes specular power in Blinn-Phong lighting model
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - shininess: exponent in the Phong equation
*/
float VERSE_blinnPhongSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess)
{
    vec3 H = normalize(viewDirection + lightDirection);
    return pow(max(0.0, dot(surfaceNormal, H)), shininess);
}

/* Computes specular power from Beckmann distribution
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - roughness: measuring surface roughness, smaller values are shinier
*/
float VERSE_beckmannSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float roughness)
{
    return VERSE_beckmannDistribution(dot(surfaceNormal, normalize(lightDirection + viewDirection)), roughness);
}

/* Computes specular power from Gaussian microfacet distribution
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - shininess: size of the specular hight light, smaller values give a sharper spot
*/
float VERSE_gaussianSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess)
{
    vec3 H = normalize(lightDirection + viewDirection);
    float theta = acos(dot(H, surfaceNormal));
    float w = theta / shininess; return exp(-w*w);
}

/* Computes specular power in Cook-Torrance lighting model
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - roughness: measuring the surface roughness, 0 for smooth, 1 for matte
   - fresnel: Fresnel exponent, 0 for no Fresnel, higher values create a rim effect around objects
*/
float VERSE_cookTorranceSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                                 float roughness, float fresnel)
{
    float VdotN = max(dot(viewDirection, surfaceNormal), 0.0);
    float LdotN = max(dot(lightDirection, surfaceNormal), 0.0);
    vec3 H = normalize(lightDirection + viewDirection);
    float NdotH = max(dot(surfaceNormal, H), 0.0);
    float VdotH = max(dot(viewDirection, H), 0.000001);
    float x = 2.0 * NdotH / VdotH;
    float G = min(1.0, min(x * VdotN, x * LdotN));
    float D = VERSE_beckmannDistribution(NdotH, roughness);
    float F = pow(1.0 - VdotN, fresnel);  // Fresnel term
    return G * F * D / max(PI * VdotN * LdotN, 0.000001);
}

/* Compute anisotropic specular power in Ward lighting model
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - fiberParallel: unit length vector tangent to the surface aligned with the local fiber orientation
   - fiberPerpendicular: unit length vector tangent to surface aligned with the local fiber orientation
   - shinyParallel: roughness of the fibers in the parallel direction
   - shinyPerpendicular: roughness of the fibers in perpendicular direction

   <Simplify>
   varying vec3 fiberDirection;
   fiberParallel = normalize(fiberDirection);
   fiberPerpendicular = normalize(cross(surfaceNormal, fiberDirection));
*/
float VERSE_wardSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
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

/* Compute vignette values from UV coordinates
   - uv: UV coordinates in the range 0 to 1
   - size: the size in the form (w/2, h/2), vec2(0.25, 0.25) will start fading in halfway between the center and edges
   - radius: vignette's radius, 0.5 results in a vignette that will just touch the edges of the UV coordinate system
   - smoothness: how quickly the vignette fades in, a value of zero resulting in a hard edge
*/
float VERSE_vignetteEffect(vec2 uv, vec2 size, float roundness, float smoothness)
{
	uv -= 0.5;  // Center UVs
	float minWidth = min(size.x, size.y);  // Shift UVs based on the larger of width/height
	uv.x = sign(uv.x) * clamp(abs(uv.x) - abs(minWidth - size.x), 0.0, 1.0);
	uv.y = sign(uv.y) * clamp(abs(uv.y) - abs(minWidth - size.y), 0.0, 1.0);
	float boxSize = minWidth * (1.0 - roundness);
	float dist = VERSE_signedDistanceSquare(uv, boxSize) - (minWidth * roundness);
	return 1.0 - smoothstep(0.0, smoothness, dist);
}
