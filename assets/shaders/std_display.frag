#version 130
uniform sampler2D ColorBuffer, SsaoBlurredBuffer;
in vec4 texCoord0;

void main()
{
	vec2 uv0 = texCoord0.xy;
	vec4 color = texture(ColorBuffer, uv0);
	float ao = texture(SsaoBlurredBuffer, uv0).r;
	gl_FragColor = vec4(color.rgb, 1.0);
}
