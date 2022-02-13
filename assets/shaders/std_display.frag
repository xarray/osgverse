#version 130
uniform sampler2D ColorBuffer;
in vec4 texCoord0;

void main()
{
	vec2 uv0 = texCoord0.xy;
	vec4 color = texture(ColorBuffer, uv0);
	gl_FragColor = color;
}
