#version 130
uniform sampler2D NormalBuffer, DepthBuffer, DiffuseMetallicBuffer;
uniform sampler2D SpecularRoughnessBuffer, EmissionOcclusionBuffer;
uniform vec2 NearFarPlanes;
in vec4 texCoord0;

void main()
{
	vec2 uv0 = texCoord0.xy;
	vec4 color = texture(DiffuseMetallicBuffer, uv0);
	gl_FragColor = color;
}
