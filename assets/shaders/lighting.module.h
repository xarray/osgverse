/* Computes diffuse intensity in Lambertian lighting model
   - lightDirection: unit length vec3 from the surface point toward the light
   - surfaceNormal: unit length normal at the sample point
*/
float lambertDiffuse(vec3 lightDirection, vec3 surfaceNormal);

/* Compute diffuse intensity in Oren-Nayar lighting model
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - roughness: measuring the surface roughness, 0 for smooth, 1 for matte
   - albedo: measuring the intensity of the diffuse reflection, >0.96 do not conserve energy
*/
float orenNayarDiffuse(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                       float roughness, float albedo);

/* Computes specular power in Phong lighting model
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - shininess: exponent in the Phong equation
*/
float phongSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess);

/* Computes specular power in Blinn-Phong lighting model
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - shininess: exponent in the Phong equation
*/
float blinnPhongSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess);

/* Computes specular power from Beckmann distribution
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - roughness: measuring surface roughness, smaller values are shinier
*/
float beckmannSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float roughness);

/* Computes specular power from Gaussian microfacet distribution
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - shininess: size of the specular hight light, smaller values give a sharper spot
*/
float gaussianSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess);

/* Computes specular power in Cook-Torrance lighting model
   - lightDirection: unit length vec3 from the surface point toward the light
   - viewDirection: unit length vec3 from the surface point toward the camera
   - surfaceNormal: unit length normal at the sample point
   - roughness: measuring the surface roughness, 0 for smooth, 1 for matte
   - fresnel: Fresnel exponent, 0 for no Fresnel, higher values create a rim effect around objects
*/
float cookTorranceSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                           float roughness, float fresnel);

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
float wardSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                   vec3 fiberParallel, vec3 fiberPerpendicular,
                   float shinyParallel, float shinyPerpendicular);

/* Compute new texture coordinates for parallax occlusion mapping
   - depthMap: depth/displacement map, only the red channel is used
   - uv: regular texture coordinates for depthMap
   - displacement: direction in which to shift the texture, usually the xy component of view direction multiplied by a depth scalar
   - pivot: elevation from which to pivot the displacement (0.0 is the highest and 1.0 is the lowest elevation)
   - layers: iteration number, suggestion value is 8
*/
vec2 parallaxOcclusionMapping(sampler2D depthMap, vec2 uv, vec2 displacement, float pivot, int layers);

/* Compute vignette values from UV coordinates
   - uv: UV coordinates in the range 0 to 1
   - size: the size in the form (w/2, h/2), vec2(0.25, 0.25) will start fading in halfway between the center and edges
   - radius: vignette's radius, 0.5 results in a vignette that will just touch the edges of the UV coordinate system
   - smoothness: how quickly the vignette fades in, a value of zero resulting in a hard edge
*/
float vignetteEffect(vec2 uv, vec2 size, float roundness, float smoothness);
