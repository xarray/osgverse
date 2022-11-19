uniform sampler2D ColorBuffer;
uniform vec2 InvBufferResolution;
in vec4 texCoord0;
out vec4 fragData;

void main()
{
	vec2 uv0 = texCoord0.xy, texelSize = InvBufferResolution;
    vec4 color = texture(ColorBuffer, uv0);
    color += texture(ColorBuffer, uv0 + vec2(texelSize.x, 0.0));
    color += texture(ColorBuffer, uv0 + vec2(0.0, texelSize.y));
    color += texture(ColorBuffer, uv0 + vec2(texelSize.x, texelSize.y));
	fragData = vec4(color.rgb * 0.25, 1.0);
}
