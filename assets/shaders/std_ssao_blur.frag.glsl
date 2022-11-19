uniform sampler2D SsaoBuffer;
uniform vec2 BlurDirection, InvScreenResolution;
uniform float BlurSharpness;

in vec4 texCoord0;
out vec4 fragData;
const float KERNEL_RADIUS = 3;

float blurFunction(vec2 uv, float r, float center_c, float center_d, inout float w_total)
{
  vec2 aoz = texture(SsaoBuffer, uv).xy;
  float c = aoz.x, d = aoz.y;
  
  const float blurSigma = float(KERNEL_RADIUS) * 0.5;
  const float blurFalloff = 1.0 / (2.0 * blurSigma * blurSigma);
  
  float ddiff = (d - center_d) * BlurSharpness;
  float w = exp2(-r * r * blurFalloff - ddiff * ddiff);
  w_total += w; return c * w;
}

void main()
{
    vec2 uv0 = texCoord0.xy;
    vec2 aoz = texture(SsaoBuffer, uv0).xy;
    float center_c = aoz.x, center_d = aoz.y;
    float c_total = center_c, w_total = 1.0;
    
    for (float r = 1.0; r <= KERNEL_RADIUS; ++r)
    {
        vec2 uv = uv0 + BlurDirection * InvScreenResolution * r;
        c_total += blurFunction(uv, r, center_c, center_d, w_total);  
    }

    for (float r = 1.0; r <= KERNEL_RADIUS; ++r)
    {
        vec2 uv = uv0 - BlurDirection * InvScreenResolution * r;
        c_total += blurFunction(uv, r, center_c, center_d, w_total);  
    }
    fragData = vec4(c_total / w_total);
}
