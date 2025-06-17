VERSE_FS_IN vec4 color, invCovariance;
VERSE_FS_IN vec2 center2D;
VERSE_FS_OUT vec4 fragData;

void main()
{
    vec2 d = gl_FragCoord.xy - center2D;  // FIXME: use texture for gaussian evaluation
    mat2 cov2Dinv = mat2(invCovariance.xy, invCovariance.zw);
    float g = exp(-0.5 * dot(d, cov2Dinv * d));

    float alpha = color.a * g;
    if (alpha <= (1.0 / 256.0)) discard;
    fragData = vec4(color.rgb * alpha, alpha);
    VERSE_FS_FINAL(fragData);
}
