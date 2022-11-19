in vec4 texCoord0, lightProjVec;
out vec4 fragData;

void main()
{
	fragData = vec4(1.0, (lightProjVec.yz / lightProjVec.w), 1.0);
}
