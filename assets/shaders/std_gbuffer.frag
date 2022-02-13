#version 130
uniform sampler2D DiffuseMap, NormalMap, SpecularMap, ShininessMap;
uniform sampler2D AmbientMap, EmissiveMap, ReflectionMap;
uniform sampler2DArray ShadowMap;
uniform mat4 lightMatrices[4];
in vec4 worldVertex, texCoord0, texCoord1;
in vec3 eyeNormal, eyeTangent, eyeBinormal;

void main()
{
    vec2 uv0 = texCoord0.xy, uv1 = texCoord1.xy;
    vec4 color = texture(DiffuseMap, uv0) * gl_Color;
    vec4 normalValue = texture(NormalMap, uv0);
    vec3 specular = texture(SpecularMap, uv0).rgb;
    vec3 emission = texture(EmissiveMap, uv1).rgb;
    vec2 metalRough = texture(ShininessMap, uv0).rg;
    float occlusion = texture(AmbientMap, uv0).r;
    
    // Compute eye-space normal
    vec3 eyeNormal2 = eyeNormal;
    if (normalValue.a > 0.1)
    {
        vec3 tsNormal = normalize(2.0 * normalValue.rgb - vec3(1.0));
        eyeNormal2 = normalize(mat3(eyeTangent, eyeBinormal, eyeNormal) * tsNormal);
    }
    
    // Shadow computation
    float shadowTerm = 1.0, shadowLayer = 0.0;  // TODO
    vec4 lightProjVec = lightMatrices[int(shadowLayer)] * worldVertex;
    vec2 lightProjUV = (lightProjVec.xy / lightProjVec.w) * 0.5 + vec2(0.5);
    {
        vec4 lightProjVec0 = texture(ShadowMap, vec3(lightProjUV.xy, shadowLayer));
        float depth = lightProjVec.z / lightProjVec.w, depth0 = lightProjVec0.z + 0.005;
        shadowTerm = (lightProjVec0.x > 0.1 && depth > depth0) ? 0.5 : 1.0;
    }
    // TODO: where to store shadowTerm?
    
    // MRT output
	gl_FragData[0]/*NormalBuffer*/ = vec4(eyeNormal2.xyz, 1.0);
	gl_FragData[1]/*DiffuseMetallicBuffer*/ = vec4(color.rgb, metalRough.r);
    gl_FragData[2]/*SpecularRoughnessBuffer*/ = vec4(specular, metalRough.g);
    gl_FragData[3]/*EmissionOcclusionBuffer*/ = vec4(emission, occlusion);
}
