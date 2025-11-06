// https://github.com/blender/blender/tree/main/source/blender/nodes/shader/nodes

vec3 rgbToHsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
    vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);
    float d = q.x - min(q.w, q.y), e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsvToRgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

/// COLOR ADJUST NODES
vec3 brightnessContrast(vec3 col, vec3 b, vec3 c)
{ return max(b + col * (c + vec3(1.0)) - (c * vec3(0.5)), vec3(0.0)); }

vec3 gamma(vec3 col, vec3 g)
{ return pow(col, g); }

vec3 setHsv(vec3 col, vec3 fac, vec3 h, vec3 s, vec3 v)
{
    vec3 hsv = rgbToHsv(col);
    hsv.x = hsv.x + (h.x - 0.5);
    if (hsv.x > 1.0) hsv.x -= 1.0;
    else if (hsv.x < 0.0) hsv.x += 1.0;
    hsv.y = clamp(hsv.y * s.x, 0.0, 1.0);
    hsv.z = clamp(hsv.z * v.x, 0.0, 1.0);

    vec3 col2 = hsvToRgb(hsv);
    return mix(col, col2, fac);
}

/// COLOR MIX NODES
vec3 mixColor(vec3 col0, vec3 col1, vec3 f) { return mix(col0, col1, f); }

/// COLOR NODES
vec3 setBlackWhite(vec3 col)
{ return vec3(dot(col, vec3(0.2126, 0.7152, 0.0722))); }

vec3 invert(vec3 col, vec3 fac)
{ return mix(col, vec3(1.0) - col, fac); }

/// UNSORTED
vec3 lightFalloff(vec3 power, vec3 sm)
{ return power / (sm + vec3(1.0)); }

vec3 normalStrength(vec3 col, vec3 s)
{ return normalize(pow(col, s)); }

vec3 mathAdd(vec3 v0, vec3 v1, vec3 v2) { return v0 + v1; }
vec3 mathSub(vec3 v0, vec3 v1, vec3 v2) { return v0 - v1; }
vec3 mathMult(vec3 v0, vec3 v1, vec3 v2) { return v0 * v1; }
