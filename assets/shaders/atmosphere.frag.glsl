uniform sampler2D Transmittance2D, Irradiance2D;
uniform sampler3D Scattering3D, SingleMie3D;
uniform vec3 CameraPosition, WhitePoint;
uniform vec3 EarthCenter, SunDirection;
uniform vec2 SunSize;
uniform float Exposure;
VERSE_FS_IN vec3 eyeSpaceRay;
VERSE_FS_OUT vec4 fragData;

const float kLengthUnitInMeters = 1000.0;
const float kSphereRadius = 1000.0 / kLengthUnitInMeters;
const vec3 kSphereCenter = vec3(0.0, 0.0, 1000.0) / kLengthUnitInMeters;
const vec3 kSphereAlbedo = vec3(0.8);
const vec3 kGroundAlbedo = vec3(0.0, 0.0, 0.04);

#ifdef RADIANCE_API_ENABLED
RadianceSpectrum GetSolarRadiance()
{
    return ATMOSPHERE.solar_irradiance / (PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius);
}

RadianceSpectrum GetSkyRadiance(Position camera, Direction view_ray, Length shadow_length,
                                Direction sun_direction, out DimensionlessSpectrum transmittance)
{
    return GetSkyRadiance(ATMOSPHERE, Transmittance2D, Scattering3D, SingleMie3D,
                          camera, view_ray, shadow_length, sun_direction, transmittance);
}

RadianceSpectrum GetSkyRadianceToPoint(Position camera, Position point, Length shadow_length,
                                       Direction sun_direction, out DimensionlessSpectrum transmittance)
{
    return GetSkyRadianceToPoint(ATMOSPHERE, Transmittance2D, Scattering3D, SingleMie3D,
                                 camera, point, shadow_length, sun_direction, transmittance);
}

IrradianceSpectrum GetSunAndSkyIrradiance(Position p, Direction normal, Direction sun_direction,
                                          out IrradianceSpectrum sky_irradiance)
{
    return GetSunAndSkyIrradiance(ATMOSPHERE, Transmittance2D, Irradiance2D,
                                  p, normal, sun_direction, sky_irradiance);
}
#endif

Luminance3 GetSolarLuminance()
{
    return ATMOSPHERE.solar_irradiance / (PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius)
         * SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
}

Luminance3 GetSkyLuminance(Position camera, Direction view_ray, Length shadow_length,
                           Direction sun_direction, out DimensionlessSpectrum transmittance)
{
    return GetSkyRadiance(ATMOSPHERE, Transmittance2D, Scattering3D, SingleMie3D,
                          camera, view_ray, shadow_length, sun_direction, transmittance) *
                          SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
}

Luminance3 GetSkyLuminanceToPoint(Position camera, Position point, Length shadow_length,
                                  Direction sun_direction, out DimensionlessSpectrum transmittance)
{
    return GetSkyRadianceToPoint(ATMOSPHERE, Transmittance2D, Scattering3D, SingleMie3D,
                                 camera, point, shadow_length, sun_direction, transmittance) *
                                 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
}

Illuminance3 GetSunAndSkyIlluminance(Position p, Direction normal, Direction sun_direction,
                                     out IrradianceSpectrum sky_irradiance)
{
    IrradianceSpectrum sun_irradiance = GetSunAndSkyIrradiance(
        ATMOSPHERE, Transmittance2D, Irradiance2D, p, normal, sun_direction, sky_irradiance);
    sky_irradiance *= SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
    return sun_irradiance * SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
}

#ifdef USE_LUMINANCE
#define GetSolarRadiance GetSolarLuminance
#define GetSkyRadiance GetSkyLuminance
#define GetSkyRadianceToPoint GetSkyLuminanceToPoint
#define GetSunAndSkyIrradiance GetSunAndSkyIlluminance
#endif

vec3 GetSolarRadiance();
vec3 GetSkyRadiance(vec3 camera, vec3 eyeSpaceRay, float shadow_length, vec3 sun_dir, out vec3 transmittance);
vec3 GetSkyRadianceToPoint(vec3 camera, vec3 point, float shadow_length, vec3 sun_dir, out vec3 transmittance);
vec3 GetSunAndSkyIrradiance(vec3 p, vec3 normal, vec3 sun_dir, out vec3 sky_irradiance);

float GetSunVisibility(vec3 point, vec3 sun_dir)
{
    vec3 p = point - kSphereCenter;
    float p_dot_v = dot(p, sun_dir), p_dot_p = dot(p, p);
    float ray_sphere_center_squared_D = p_dot_p - p_dot_v * p_dot_v;
    float D_to_intersection = -p_dot_v - sqrt(kSphereRadius * kSphereRadius - ray_sphere_center_squared_D);
    if (D_to_intersection > 0.0)
    {
        // Compute the distance between the view ray and the sphere, and the corresponding
        // (tangent of the) subtended angle. Finally, use this to compute an approximate sun visibility.
        float ray_sphere_distance = kSphereRadius - sqrt(ray_sphere_center_squared_D);
        float ray_sphere_angular_distance = -ray_sphere_distance / p_dot_v;
        return smoothstep(1.0, 0.0, ray_sphere_angular_distance / SunSize.x);
    }
    return 1.0;
}

float GetSkyVisibility(vec3 point)
{
    vec3 p = point - kSphereCenter; float p_dot_p = dot(p, p);
    return 1.0 + p.z / sqrt(p_dot_p) * kSphereRadius * kSphereRadius / p_dot_p;
}

void GetSphereShadowInOut(vec3 view_dir, vec3 sun_dir, out float d_in, out float d_out)
{
    vec3 pos = CameraPosition - kSphereCenter;
    float pos_dot_sun = dot(pos, sun_dir), view_dot_sun = dot(view_dir, sun_dir);
    float k = SunSize.x; float l = 1.0 + k * k;
    float a = 1.0 - l * view_dot_sun * view_dot_sun;
    float b = dot(pos, view_dir) - l * pos_dot_sun * view_dot_sun - k * kSphereRadius * view_dot_sun;
    float c = dot(pos, pos) - l * pos_dot_sun * pos_dot_sun
            - 2.0 * k * kSphereRadius * pos_dot_sun - kSphereRadius * kSphereRadius;
    float discriminant = b * b - a * c;
    if (discriminant > 0.0)
    {
        d_in = max(0.0, (-b - sqrt(discriminant)) / a);
        d_out = (-b + sqrt(discriminant)) / a;
        float d_base = -pos_dot_sun / view_dot_sun;
        float d_apex = -(pos_dot_sun + kSphereRadius / k) / view_dot_sun;
        if (view_dot_sun > 0.0)
        {
            d_in = max(d_in, d_apex);
            d_out = a > 0.0 ? min(d_out, d_base) : d_base;
        }
        else
        {
            d_in = a > 0.0 ? max(d_in, d_base) : d_base;
            d_out = min(d_out, d_apex);
        }
    }
    else { d_in = 0.0; d_out = 0.0; }
}

void main()
{
    vec3 view_direction = normalize(eyeSpaceRay);
    float fragment_angular_size = length(dFdx(eyeSpaceRay) + dFdy(eyeSpaceRay)) / length(eyeSpaceRay);

    float shadow_in = 0.0, shadow_out = 0.0;
    GetSphereShadowInOut(view_direction, SunDirection, shadow_in, shadow_out);

    // Hack to fade out light shafts when the Sun is very close to the horizon.
    float fadein_hack = smoothstep(0.02, 0.04, dot(normalize(CameraPosition - EarthCenter), SunDirection));

    // Compute the distance between the view ray line and the sphere center,
    // and the distance between the camera and the intersection of the view
    // ray with the sphere (or NaN if there is no intersection).
    vec3 p = CameraPosition - kSphereCenter;
    float p_dot_v = dot(p, view_direction), p_dot_p = dot(p, p);
    float ray_sphere_center_squared_D = p_dot_p - p_dot_v * p_dot_v;
    float D_to_intersection = -p_dot_v - sqrt(kSphereRadius * kSphereRadius - ray_sphere_center_squared_D);

    // Compute the radiance reflected by the sphere, if the ray intersects it.
    float sphere_alpha = 0.0; vec3 sphere_radiance = vec3(0.0);
    if (D_to_intersection > 0.0)
    {
        // Compute the distance between the view ray and the sphere, and the
        // corresponding (tangent of the) subtended angle. Finally, use this to
        // compute the approximate analytic antialiasing factor sphere_alpha.
        float ray_sphere_distance = kSphereRadius - sqrt(ray_sphere_center_squared_D);
        float ray_sphere_angular_distance = -ray_sphere_distance / p_dot_v;
        sphere_alpha = min(ray_sphere_angular_distance / fragment_angular_size, 1.0);

        vec3 point = CameraPosition + view_direction * D_to_intersection;
        vec3 normal = normalize(point - kSphereCenter), sky_irradiance, transmittance;
        vec3 sun_irradiance = GetSunAndSkyIrradiance(
            point - EarthCenter, normal, SunDirection, sky_irradiance);
        sphere_radiance = kSphereAlbedo * (1.0 / PI) * (sun_irradiance + sky_irradiance);

        float shadow_length =
            max(0.0, min(shadow_out, distance_to_intersection) - shadow_in) * fadein_hack;
        vec3 in_scatter = GetSkyRadianceToPoint(CameraPosition - EarthCenter, point - EarthCenter,
                                                shadow_length, SunDirection, transmittance);
        sphere_radiance = sphere_radiance * transmittance + in_scatter;
    }

    // Compute the distance between the view ray line and the Earth center,
    // and the distance between the camera and the intersection of the view
    // ray with the ground (or NaN if there is no intersection).
    p = CameraPosition - EarthCenter;
    p_dot_v = dot(p, view_direction); p_dot_p = dot(p, p);
    float ray_EarthCenter_squared_D = p_dot_p - p_dot_v * p_dot_v;
    D_to_intersection =
        -p_dot_v - sqrt(EarthCenter.z * EarthCenter.z - ray_EarthCenter_squared_D);

    // Compute the radiance reflected by the ground, if the ray intersects it.
    float ground_alpha = 0.0; vec3 ground_radiance = vec3(0.0);
    if (D_to_intersection > 0.0)
    {
        vec3 point = CameraPosition + view_direction * D_to_intersection;
        vec3 normal = normalize(point - EarthCenter);
        vec3 sky_irradiance, transmittance;

        // Compute the radiance reflected by the ground.
        vec3 sun_irradiance = GetSunAndSkyIrradiance(
            point - EarthCenter, normal, SunDirection, sky_irradiance);
        ground_radiance = kGroundAlbedo * (1.0 / PI) * (
            sun_irradiance * GetSunVisibility(point, SunDirection) +
            sky_irradiance * GetSkyVisibility(point));

        float shadow_length = max(0.0, min(shadow_out, D_to_intersection) - shadow_in) * fadein_hack;
        vec3 in_scatter = GetSkyRadianceToPoint(CameraPosition - EarthCenter, point - EarthCenter,
                                                shadow_length, SunDirection, transmittance);
        ground_radiance = ground_radiance * transmittance + in_scatter;
        ground_alpha = 1.0;
    }

    // Compute the radiance of the sky.
    float shadow_length = max(0.0, shadow_out - shadow_in) * fadein_hack;
    vec3 transmittance; vec3 radiance = GetSkyRadiance(
        CameraPosition - EarthCenter, view_direction, shadow_length, SunDirection, transmittance);

    // If the view ray intersects the Sun, add the Sun radiance.
    if (dot(view_direction, SunDirection) > SunSize.y)
        radiance = radiance + transmittance * GetSolarRadiance();
    radiance = mix(radiance, ground_radiance, ground_alpha);
    radiance = mix(radiance, sphere_radiance, sphere_alpha);
    fragData.rgb = pow(vec3(1.0) - exp((-radiance / WhitePoint) * Exposure), vec3(1.0 / 2.2));
    fragData.a = 1.0; VERSE_FS_FINAL(fragData);
}
