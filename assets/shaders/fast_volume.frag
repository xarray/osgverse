uniform mat4 osg_ViewMatrix, RotationOffset;
uniform sampler3D VolumeTexture;
uniform sampler1D TransferTexture;
uniform vec3 Color, BoundingMin, BoundingMax;
uniform vec3 SliceMin, SliceMax;
uniform vec3 ValueRange;  // (min, max-min, invalid)
uniform int RayMarchingSamples, TransferMode;
uniform float DensityFactor, DensityPower;
VERSE_FS_IN vec4 eyeVertex, texCoord;
VERSE_FS_OUT vec4 fragData;

const int maxSamples = 128;
vec2 rayIntersectBox(vec3 rayDirection, vec3 rayOrigin)
{
    // Intersect ray with bounding box
    vec3 rayInvDirection = 1.0 / rayDirection;
    vec3 bbMinDiff = (BoundingMin - rayOrigin) * rayInvDirection;
    vec3 bbMaxDiff = (BoundingMax - rayOrigin) * rayInvDirection;
    vec3 imax = max(bbMaxDiff, bbMinDiff);
    vec3 imin = min(bbMaxDiff, bbMinDiff);
    float back = min(imax.x, min(imax.y, imax.z));
    float front = max(max(imin.x, 0.0), max(imin.y, imin.z));
    return vec2(back, front);
}

float remap(float x, float deadZone, float outerScale, float k)
{
    return outerScale * tanh((x - deadZone * sign(x)) * k);
}

void main()
{
    // Get object-space ray origin & direction of each fragment
    mat4 invModelView = transpose(osg_ViewMatrix * RotationOffset);
    vec4 camPos = -vec4((osg_ViewMatrix * RotationOffset)[3]);
    vec3 rayDirection = normalize((invModelView * eyeVertex).xyz);
    vec3 rayOrigin = (invModelView * camPos).xyz;

    // Intersect ray with the volume's bounding box
    // - subtract small increment to avoid errors on front boundary
    // - discard points outside the box (no intersection)
    vec2 intersection = rayIntersectBox(rayDirection, rayOrigin);
    intersection.y -= 0.000001;
    if (intersection.x <= intersection.y) discard;

    float stepSize = 1.732 / float(RayMarchingSamples);
    vec3 rayStart = rayOrigin + rayDirection * intersection.y;
    vec3 rayStop = rayOrigin + rayDirection * intersection.x;
    vec3 step = (rayStop - rayStart) * stepSize;
    vec3 pos = rayStart, invBoundingDiff = 1.0 / (BoundingMax - BoundingMin);

    // Raymarch, front to back
    float T = 1.0, travel = distance(rayStop, rayStart) / stepSize;
    float factor = DensityFactor * stepSize, totalDensity = 0.0;
    int samples = int(ceil(travel));
    vec4 resultColor = vec4(0.0);
    for (int i = 0; i < maxSamples; ++i)
    {
        vec3 uv = (pos - BoundingMin) * invBoundingDiff;
        float density = VERSE_TEX3D(VolumeTexture, uv).r;
        if ((T > 0.99 && density == ValueRange.z) ||
            any(lessThan(uv, SliceMin)) || any(greaterThan(uv, SliceMax)))
        { density = 0.0; }
        else
        {
            density = remap(density, 1.0, ValueRange.y * 0.5, DensityPower);
            density = (density - ValueRange.x) / ValueRange.y;
            density = clamp(density, 0.0, 1.0);
        }

        vec4 value = vec4(0.0); totalDensity += density;
        if (TransferMode == 1) value = VERSE_TEX1D(TransferTexture, density);
        else value = vec4(density); value.a = density;
        value *= factor; resultColor += T * value;
        T *= 1.0 - value.a; pos += step;
        if (i == samples - 1 || T < 0.01) break;
    }

    if (totalDensity < 0.01) discard;
    fragData = vec4(Color * resultColor.rgb, 1.0 - T);
    VERSE_FS_FINAL(fragData);
}
