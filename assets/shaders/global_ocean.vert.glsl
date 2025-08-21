uniform sampler1D WavesSampler; // waves parameters (h, omega, kx, ky) in wind space
uniform mat4 CameraToOcean, OceanToCamera, OceanToWorld;
uniform mat4 ScreenToCamera, CameraToScreen;
uniform vec4 SeaGridLODs;  // grid cell size in pixels, angle under which a grid cell is seen,
                           // and parameters of the geometric series used for wavelengths
uniform vec3 SeaColor; // sea bottom color
uniform vec3 OceanCameraPos, OceanSunDir;
uniform vec3 Horizon1, Horizon2;
uniform float Radius, WaveCount; // number of waves
uniform float HeightOffset; // so that surface height is centered around z = 0
uniform float SeaRoughness; // total variance
uniform float osg_SimulationTime;

#define NYQUIST_MIN 0.5 // wavelengths below NYQUIST_MIN * sampling period are fully attenuated
#define NYQUIST_MAX 1.25 // wavelengths above NYQUIST_MAX * sampling period are not attenuated at all
const float PI = 3.141592657;
const float g = 9.81;

VERSE_VS_OUT vec3 oceanUv; // coordinates in wind space used to compute P(u); z: undersea flag
VERSE_VS_OUT vec3 oceanP; // wave point P(u) in ocean space
VERSE_VS_OUT vec3 oceanDPdu; // dPdu in wind space, used to compute N
VERSE_VS_OUT vec3 oceanDPdv; // dPdv in wind space, used to compute N
VERSE_VS_OUT float oceanSigmaSq; // variance of unresolved waves in wind space
VERSE_VS_OUT float oceanLod, oceanValid;

vec2 oceanPos(vec3 vertex, bool checkValid, out float t, out vec3 cameraDir, out vec3 oceanDir)
{
    float horizon = Horizon1.x + Horizon1.y * vertex.x -
                    sqrt(Horizon2.x + (Horizon2.y + Horizon2.z * vertex.x) * vertex.x);
    cameraDir = normalize((ScreenToCamera * vec4(vertex.x, min(vertex.y, horizon), 0.0, 1.0)).xyz);
    oceanDir = (CameraToOcean * vec4(cameraDir, 0.0)).xyz;

    float cz = OceanCameraPos.z, dz = oceanDir.z;
    if (Radius == 0.0)
        t = (HeightOffset + 5.0 - cz) / dz;
    else
    {
        float b = dz * (cz + Radius), c = cz * (cz + 2.0 * Radius);
        float tSphere = -b - sqrt(max(b * b - c, 0.0));
        float tApprox = -cz / dz * (1.0 + cz / (2.0 * Radius) * (1.0 - dz * dz));
        t = abs((tApprox - tSphere) * dz) < 1.0 ? tApprox : tSphere;

        // Approx. check if camera-dir intersects with sphere
        if (checkValid) oceanValid = (b * b - c < 0.0) ? -1.0 : 1.0;
    }
    return OceanCameraPos.xy + t * oceanDir.xy;
}

vec2 oceanPos(vec3 vertex)
{
    float t = 0.0; vec3 cameraDir, oceanDir;
    return oceanPos(vertex, false, t, cameraDir, oceanDir);
}

void main()
{
    float t = 0.0; vec3 cameraDir, oceanDir; oceanValid = 1.0;
    oceanUv.z = (OceanCameraPos.z > 0.0) ? 1.0 : 0.0;  // 'over the sea' or 'under-sea'
    vec2 uv = oceanPos(osg_Vertex.xyz, true, t, cameraDir, oceanDir);
    oceanValid = -1.0; // if (OceanCameraPos.z > 500000.0) oceanValid = -1.0;  // no need to check if far enough

    float lod = -t / oceanDir.z * SeaGridLODs.y; // size in meters of one grid cell, projected on the sea surface
    vec2 duv = oceanPos(osg_Vertex.xyz + vec3(0.0, 0.01, 0.0)) - uv;
    vec3 dP = vec3(0.0, 0.0, HeightOffset + (Radius > 0.0 ? 0.0 : 5.0));
    vec3 dPdu = vec3(1.0, 0.0, 0.0), dPdv = vec3(0.0, 1.0, 0.0);

    float sigmaSq = SeaRoughness, T = osg_SimulationTime * 3.0;
    if (duv.x != 0.0 || duv.y != 0.0)
    {
        float iMin = max(floor((log2(NYQUIST_MIN * lod) - SeaGridLODs.z) * SeaGridLODs.w), 0.0);
        for (float i = iMin; i < int(WaveCount); ++i)
        {
            vec4 wt = VERSE_TEX1D(WavesSampler, (i + 0.5) / WaveCount);
            //vec4 wt = textureLod(WavesSampler, (i + 0.5) / WaveCount, 0.0);
            float phase = wt.y * T - dot(wt.zw, uv);
            float s = sin(phase), c = cos(phase);
            float overk = g / (wt.y * wt.y);
            float wm = smoothstep(NYQUIST_MIN, NYQUIST_MAX, (2.0 * PI) * overk / lod);
            vec3 factor = wm * wt.x * vec3(wt.zw * overk, 1.0);
            dP += factor * vec3(s, s, c);

            vec3 dPd = factor * vec3(c, c, -s);
            dPdu -= dPd * wt.z; dPdv -= dPd * wt.w;
            wt.zw *= overk;

            float kh = wt.x / overk;
            sigmaSq -= wt.z * wt.z * (1.0 - sqrt(1.0 - kh * kh));
        }
    }

    vec3 p = t * oceanDir + dP + vec3(0.0, 0.0, OceanCameraPos.z);
    if (Radius > 0.0)
    {
        dPdu += vec3(0.0, 0.0, -p.x / (Radius + p.z));
        dPdv += vec3(0.0, 0.0, -p.y / (Radius + p.z));
    }

    oceanLod = lod; oceanUv.xy = uv; oceanP = p;
    oceanDPdu = dPdu; oceanDPdv = dPdv; oceanSigmaSq = sigmaSq;
    vec4 pos = CameraToScreen * vec4(t * cameraDir + (OceanToCamera * vec4(dP, 1.0)).xyz, 1.0);
    gl_Position = pos / pos.w; gl_Position.z = 0.0;
}
