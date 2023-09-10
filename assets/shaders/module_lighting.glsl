#define M_PI 3.1415926535897932384626433832795
#define NUM_IBL_SAMPLES 32

float clamped_cosine(vec3 a, vec3 b)
{ return min(max(dot(a, b), 0.0), 1.0); }

float normal_distribution_function(vec3 normal, vec3 halfway, float roughness)
{
    float alpha_2 = pow(roughness, 4.0);
    return alpha_2 / (M_PI * pow((pow(dot(normal, halfway), 2.0) *
           (alpha_2 - 1.0) + 1.0), 2.0));
}

float get_geometric_attenuation(vec3 direction, vec3 view, vec3 normal,
                                vec3 halfway, float roughness)
{
    float k = pow(roughness + 1.0, 2.0) / 8.0;
    float g1l = dot(normal, direction) / (dot(normal, direction) * (1.0 - k) + k);
    float g1v = dot(normal, view) / (dot(normal, view) * (1.0 - k) + k);
    return g1l * g1v;
}

vec3 get_fresnel(vec3 view, vec3 halfway, vec3 f0)
{
    float dot_vh = dot(view, halfway);
    return f0 + (1.0 - f0) * pow(2.0, (-5.55473 * dot_vh - 6.98316) * dot_vh);
}

vec3 get_light_contribution_helper(vec3 albedo, vec3 f0, float roughness, vec3 normal,
                                   vec3 incoming_irradiance, vec3 view_dir, vec3 light_dir)
{
    vec3 view = normalize(view_dir);  // view_dir = cam_pos - obj_pos
    vec3 halfway = normalize(light_dir + view);  // light_dir = light_pos - obj_pos
    vec3 albedo_contribution = albedo / M_PI;

    float d = normal_distribution_function(normal, halfway, roughness);
    vec3 f = get_fresnel(view, halfway, f0);
    float g = get_geometric_attenuation(light_dir, view, normal, halfway, roughness);
    vec3 specular_contribution = (d * f * g) / (dot(normal, light_dir) * dot(normal, view));
    return (albedo_contribution + specular_contribution) * incoming_irradiance;
}

vec3 get_light_contribution(vec3 albedo, float metallic, float roughness, vec3 normal,
                            vec3 incoming_irradiance, vec3 view_dir, vec3 light_dir)
{
    vec3 dielectric_contribution = get_light_contribution_helper(
        albedo, vec3(0.04, 0.04, 0.04), roughness, normal, incoming_irradiance, view_dir, light_dir);
    vec3 metallic_contribution = get_light_contribution_helper(
        vec3(0.0, 0.0, 0.0), albedo, roughness, normal, incoming_irradiance, view_dir, light_dir);
    return mix(dielectric_contribution, metallic_contribution, metallic);
}

vec3 get_directional_light_contribution(vec3 view_dir, vec3 light_dir0, vec3 light_irradiance,
                                        vec3 albedo, float metallic, float roughness, vec3 normal)
{
    vec3 light_dir = -light_dir0, incoming_irradiance = light_irradiance * clamped_cosine(normal, light_dir);
    return get_light_contribution(albedo, metallic, roughness, normal, incoming_irradiance, view_dir, light_dir);
}

vec3 get_point_light_contribution(vec3 view_dir, vec3 eyespace_pos, vec3 light_pos, vec3 light_irradiance,
                                  vec3 albedo, float metallic, float roughness, vec3 normal)
{
    vec3 light_dir = -normalize(light_pos - eyespace_pos);
    float dist = distance(eyespace_pos, light_pos);
    float falloff = 1.0 / (dist * dist + 1.0), light_radius = 100.0;
    falloff = pow(clamp(1.0 - pow(dist / light_radius, 4.0), 0.0, 1.0), 2.0) / (pow(dist, 2.0) + 1.0);

    vec3 incoming_irradiance = (light_irradiance * falloff) * clamped_cosine(normal, light_dir);
    return get_light_contribution(albedo, metallic, roughness, normal, incoming_irradiance, view_dir, light_dir);
}

vec3 get_ibl_sample_contribution(sampler2D envMap, vec3 view_dir, vec2 hammersley, vec3 albedo,
                                 float metallic, float roughness, vec3 normal)
{
    // Get the sample direction from the Hammersley point
    float alpha = pow(roughness, 2.0), phi = 2.0 * M_PI * hammersley.y;
    float theta = atan((alpha * sqrt(hammersley.x)) / sqrt(1.0 - hammersley.x));
    vec3 h_tangent = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));

    // This direction is in tangent space, so we need to transform it to world space.
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent_x = normalize(cross(up, normal));
    vec3 tangent_y = cross(normal, tangent_x);
    vec3 h = normalize(tangent_x * h_tangent.x + tangent_y * h_tangent.y + normal * h_tangent.z);
    vec3 v = normalize(view_dir);  // view_dir = cam_pos - obj_pos
    vec3 l = normalize(h - v), n = normal;

    // Calculate the light contribution from the sample and the environment map.
    if (clamped_cosine(n, l) > 0)
    {
        float lod = mix(6.0, 0.0, metallic);
        vec2 uv1 = vec2((1.0 + atan(l.x, l.z) / M_PI) / 2.0, acos(l.y) / M_PI);
        vec3 sample_color = vec3(textureLod(envMap, uv1, lod));

        vec3 f0 = mix(vec3(0.04, 0.04, 0.04), albedo, metallic);
        vec3 f = get_fresnel(v, h, f0);
        float g = get_geometric_attenuation(l, v, n, h, roughness);
        return (f * g * sample_color * clamped_cosine(v, h)) /
               (clamped_cosine(n, h) * clamped_cosine(n, v));
    }
    return vec3(0.0, 0.0, 0.0);
}
