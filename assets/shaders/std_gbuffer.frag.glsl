#version 130
uniform sampler2D DiffuseMap, NormalMap, SpecularMap, ShininessMap;
uniform sampler2D AmbientMap, EmissiveMap, ReflectionMap;
uniform float ModelIndicator;
in vec4 texCoord0, texCoord1, color;
in vec3 eyeNormal, eyeTangent, eyeBinormal;

void main()
{
    vec2 uv0 = texCoord0.xy, uv1 = texCoord1.xy;
    vec4 diffuse = texture(DiffuseMap, uv0) * color;
    vec4 normalValue = texture(NormalMap, uv0);
    vec3 specular = texture(SpecularMap, uv0).rgb;
    vec3 emission = texture(EmissiveMap, uv1).rgb;
    vec2 metalRough = texture(ShininessMap, uv0).rg;
    float occlusion = texture(AmbientMap, uv0).r;
    if (diffuse.a < 0.1) discard;
    
    // Compute eye-space normal
    vec3 eyeNormal2 = eyeNormal;
    if (normalValue.a > 0.1)
    {
        vec3 tsNormal = normalize(2.0 * normalValue.rgb - vec3(1.0));
        eyeNormal2 = normalize(mat3(eyeTangent, eyeBinormal, eyeNormal) * tsNormal);
    }
    
    // MRT output
	gl_FragData[0]/*NormalBuffer*/ = vec4(eyeNormal2.xyz, ModelIndicator * 0.1);
	gl_FragData[1]/*DiffuseMetallicBuffer*/ = vec4(diffuse.rgb, metalRough.g);
    gl_FragData[2]/*SpecularRoughnessBuffer*/ = vec4(specular, metalRough.r);
    gl_FragData[3]/*EmissionOcclusionBuffer*/ = vec4(emission, occlusion);
}
