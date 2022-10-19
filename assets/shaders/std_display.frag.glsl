#version 130
uniform sampler2D ColorBuffer, SsaoBlurredBuffer;
in vec4 texCoord0;
out vec4 fragData;

void main()
{
	vec2 uv0 = texCoord0.xy;
	vec4 color = texture(ColorBuffer, uv0);
	float ao = texture(SsaoBlurredBuffer, uv0).r;
	fragData = vec4(color.rgb * ao, 1.0);
}
