// LIGHTING
float lambertDiffuse(vec3 lightDirection, vec3 surfaceNormal);
float orenNayarDiffuse(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                       float roughness, float albedo);
float phongSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess);
float blinnPhongSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess);
float beckmannSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float roughness);
float gaussianSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess);
float cookTorranceSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                           float roughness, float fresnel);
float wardSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                   vec3 fiberParallel, vec3 fiberPerpendicular,
                   float shinyParallel, float shinyPerpendicular);

// EFFECTS
float vignetteEffect(vec2 uv, vec2 size, float roundness, float smoothness);
