#version 130
#define MAX_SIZE 5
#define MAX_KERNEL_SIZE ((MAX_SIZE * 2 + 1) * (MAX_SIZE * 2 + 1))
uniform sampler2D SsaoBuffer;
in vec4 texCoord0;

float blurParameter = 1.0;
vec3 valueRatios = vec3(0.3, 0.59, 0.11);
vec2 texSize = textureSize(SsaoBuffer, 0).xy;
vec2 uv0 = gl_FragCoord.xy / texSize;
int i = 0, j = 0, count = 0;
float values[MAX_KERNEL_SIZE];

vec4 color = vec4(0.0), meanTemp = vec4(0.0), mean = vec4(0.0);
float valueMean = 0.0, variance = 0.0, minVariance = -1.0;

void findMean(int i0, int i1, int j0, int j1)
{
    meanTemp = vec4(0); count = 0;
    for (i = i0; i <= i1; ++i)
    {
        for (j = j0; j <= j1; ++j)
        {
            color = texture(SsaoBuffer, uv0 + (vec2(i, j) / texSize));
            values[count] = dot(color.rgb, valueRatios);
            meanTemp += color; count += 1;
        }
    }
    
    meanTemp.rgb /= count;
    valueMean = dot(meanTemp.rgb, valueRatios);
    for (i = 0; i < count; ++i) variance += pow(values[i] - valueMean, 2.0);

    variance /= count;
    if (variance < minVariance || minVariance <= -1)
    { mean = meanTemp; minVariance = variance; }
}

void main()
{
    int size = int(blurParameter);
    findMean(-size, 0, -size, 0);  // Lower Left
    findMean(0, size, 0, size);  // Upper Right
    findMean(-size, 0, 0, size);  // Upper Left
    findMean(0, size, -size, 0);  // Lower Right
	gl_FragColor = vec4(mean.rgb, 1.0);
}
