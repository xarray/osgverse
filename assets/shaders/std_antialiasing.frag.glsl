uniform sampler2D ColorBuffer;
uniform vec2 InvScreenResolution;
in vec4 texCoord0;
out vec4 fragData;

// FXAA optimized version for mobile
#define FXAA_REDUCE_MIN   (1.0 / 128.0)
#define FXAA_REDUCE_MUL   (1.0 / 8.0)
#define FXAA_SPAN_MAX     8.0
vec4 fxaa(in sampler2D tex, in vec2 fragCoord,
          in vec2 v_rgbNW, in vec2 v_rgbNE, in vec2 v_rgbSW, in vec2 v_rgbSE, in vec2 v_rgbM)
{
    vec3 rgbNW = texture(tex, v_rgbNW).xyz;
    vec3 rgbNE = texture(tex, v_rgbNE).xyz;
    vec3 rgbSW = texture(tex, v_rgbSW).xyz;
    vec3 rgbSE = texture(tex, v_rgbSE).xyz;
    vec4 texColor = texture(tex, v_rgbM);
    vec3 rgbM  = texColor.xyz;
    vec3 luma = vec3(0.299, 0.587, 0.114);
    
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    mediump vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                          (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
              max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
              dir * rcpDirMin)) * InvScreenResolution;
    
    vec3 rgbA = 0.5 * (texture(tex, fragCoord + dir * (1.0 / 3.0 - 0.5)).xyz +
                       texture(tex, fragCoord + dir * (2.0 / 3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(tex, fragCoord + dir * -0.5).xyz +
                                     texture(tex, fragCoord + dir * 0.5).xyz);
    float lumaB = dot(rgbB, luma);
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
        return vec4(rgbA, texColor.a);
    else
        return vec4(rgbB, texColor.a);
}

void main()
{
	vec2 uv0 = texCoord0.xy;
    vec2 v_rgbNW = (uv0 + vec2(-1.0, -1.0) * InvScreenResolution);
	vec2 v_rgbNE = (uv0 + vec2(1.0, -1.0) * InvScreenResolution);
	vec2 v_rgbSW = (uv0 + vec2(-1.0, 1.0) * InvScreenResolution);
	vec2 v_rgbSE = (uv0 + vec2(1.0, 1.0) * InvScreenResolution);
    fragData = fxaa(ColorBuffer, uv0, v_rgbNW, v_rgbNE, v_rgbSW, v_rgbSE, uv0);
}
