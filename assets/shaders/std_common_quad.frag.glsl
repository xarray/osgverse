uniform sampler2D ColorBuffer;
in vec4 texCoord0;
out vec4 fragData;

void main()
{
    vec2 uv0 = texCoord0.xy;
    vec4 color = texture(ColorBuffer, uv0);
    fragData = vec4(color.rgb, 1.0);
}
