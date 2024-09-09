// LIGHTING
float VERSE_lambertDiffuse(vec3 lightDirection, vec3 surfaceNormal);
float VERSE_orenNayarDiffuse(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                             float roughness, float albedo);
float VERSE_phongSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess);
float VERSE_blinnPhongSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess);
float VERSE_beckmannSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float roughness);
float VERSE_gaussianSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float shininess);
float VERSE_cookTorranceSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                                 float roughness, float fresnel);
float VERSE_wardSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                         vec3 fiberParallel, vec3 fiberPerpendicular,
                         float shinyParallel, float shinyPerpendicular);

// EFFECTS
float VERSE_vignetteEffect(vec2 uv, vec2 size, float roundness, float smoothness);
