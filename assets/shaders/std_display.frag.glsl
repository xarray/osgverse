#version 130
uniform sampler2D ColorBuffer, DepthBuffer;
uniform mat4 GBufferMatrices[4];  // w2v, v2w, v2p, p2v
uniform vec2 FogDistance;
uniform vec3 FogColor;
uniform vec3 ColorAttribute;     // (Brightness, Saturation, Contrast)
uniform vec3 ColorBalance;       // (Cyan-Red, Magenta-Green, Yellow-Blue)
uniform int ColorBalanceMode;    // 0 - Shadow, 1 - Midtone, 2 - Highlight
uniform float VignetteRadius;
uniform float VignetteDarkness;
in vec4 texCoord0;
out vec4 fragData;

// Color balance copied from gimp/app/base/color-balance.c
float oneColorBalanceFunc(float v, float factor, int mode)
{
    if (factor == 0.0) return v;
    else if (mode == 0)
    {
        float lowValue = 1.075 - 1.0 / ((factor > 0.0 ? v : 1.0 - v) * 16.0 + 1.0);
        return clamp(v + factor * lowValue, 0.0, 1.0);
    }
    else if (mode == 1)
    {
        float midValue = 0.667 * (1.0 - pow((v - 0.5) * 2.0, 2.0));
        return clamp(v + factor * midValue, 0.0, 1.0);
    }
    else if (mode == 2)
    {
        float lowValue = 1.075 - 1.0 / ((factor > 0.0 ? 1.0 - v : v) * 16.0 + 1.0);
        return clamp(v + factor * lowValue, 0.0, 1.0);
    }
}

vec3 colorBalanceFunc(vec3 color, float cyan_red, float magenta_green, float yellow_blue, int mode)
{
    return vec3(oneColorBalanceFunc(color.r, cyan_red, mode),
                oneColorBalanceFunc(color.g, magenta_green, mode),
                oneColorBalanceFunc(color.b, yellow_blue, mode));
}

// Brightness, saturation, and contrast
vec3 colorAdjustmentFunc(vec3 color, float brt, float sat, float con)
{
    // Increase or decrease theese values to adjust r, g and b color channels seperately
    const float avgLumR = 0.5;
    const float avgLumG = 0.5;
    const float avgLumB = 0.5;
    const vec3 lumCoeff = vec3(0.2125, 0.7154, 0.0721);

    vec3 avgLumin = vec3(avgLumR, avgLumG, avgLumB);
    vec3 brtColor = color * brt;
    vec3 intensity = vec3(dot(brtColor, lumCoeff));
    vec3 satColor = mix(intensity, brtColor, sat);
    vec3 conColor = mix(avgLumin, satColor, con);
    return conColor;
}

// Vignette effect
vec3 vignetteEffectFunc(vec3 color, vec2 uv)
{
    float vignette = 1.0 - dot(uv, uv);
    return color.rgb * clamp(pow(vignette, VignetteRadius) - VignetteDarkness, 0.0, 1.0);
}

void main()
{
	vec2 uv0 = texCoord0.xy;
	vec3 colorRGB = texture(ColorBuffer, uv0).rgb;
    float depthValue = texture(DepthBuffer, uv0).r * 2.0 - 1.0;

    // Rebuild world vertex attributes
    vec4 vecInProj = vec4(uv0.x * 2.0 - 1.0, uv0.y * 2.0 - 1.0, depthValue, 1.0);
    vec4 eyeVertex = GBufferMatrices[3] * vecInProj;
    if (FogDistance.y > 0.0)
    {
        float fogFactor = (FogDistance.y - abs(eyeVertex.z / eyeVertex.w)) / (FogDistance.y - FogDistance.x);
        colorRGB = mix(FogColor, colorRGB, clamp(fogFactor, 0.0, 1.0));
    }

    // Color grading work
    colorRGB = colorBalanceFunc(colorRGB, ColorBalance.x, ColorBalance.y, ColorBalance.z, ColorBalanceMode);
    colorRGB = colorAdjustmentFunc(colorRGB, ColorAttribute.x, ColorAttribute.y, ColorAttribute.z);
    colorRGB = vignetteEffectFunc(colorRGB, uv0 - vec2(0.5, 0.5));
	fragData = vec4(pow(colorRGB, vec3(1.0 / 2.2)), 1.0);
}
