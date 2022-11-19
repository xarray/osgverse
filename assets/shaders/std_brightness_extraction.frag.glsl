uniform sampler2D ColorBuffer;
uniform float BrightnessThreshold;
in vec4 texCoord0;
out vec4 fragData;

float luminance(vec3 color)
{
    return dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
	vec2 uv0 = texCoord0.xy;
    float lum = luminance(texture(ColorBuffer, uv0).xyz);
	fragData = vec4((lum > BrightnessThreshold) ? vec3(lum) : vec3(0.0), 1.0);
}
