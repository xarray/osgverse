// https://github.com/blender/blender/tree/main/source/blender/nodes/texture/nodes

vec3 mixColor(vec3 col0, vec3 col1, vec3 f)
{
    return mix(col0, col1, f);
}

vec3 lightFalloff(vec3 power, vec3 sm)
{
    return smoothstep(vec3(0.0), power, sm);
}

vec3 normalStrength(vec3 col, vec3 s)
{
    return normalize(pow(col, s));
}

vec3 brightnessContrast(vec3 col, vec3 b, vec3 c)
{
    return mix(vec3(0.5), col * (b + vec3(1.0)), c + vec3(1.0));
}

vec3 setHsv(vec3 col, vec3 fac, vec3 h, vec3 s, vec3 v)
{
    return col;
}

vec3 setBlackWhite(vec3 col)
{
    return col;
}

vec3 invert(vec3 col, vec3 fac)
{
    return col;
}

vec3 gamma(vec3 col, vec3 g)
{
    return col;
}
