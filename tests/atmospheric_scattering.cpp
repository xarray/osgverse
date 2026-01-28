#include <osg/io_utils>
#include <osg/ImageSequence>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>
#include <functional>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

namespace atmosphere
{
    // The conversion factor between watts and lumens.
    constexpr double MAX_LUMINOUS_EFFICACY = 683.0;
    constexpr double kLambdaR = 680.0;
    constexpr double kLambdaG = 550.0;
    constexpr double kLambdaB = 440.0;
    constexpr double kPi = 3.1415926;
    constexpr double kSunAngularRadius = 0.00935 / 2.0;
    constexpr double kSunSolidAngle = kPi * kSunAngularRadius * kSunAngularRadius;
    constexpr double kLengthUnitInMeters = 1000.0;

    // Values from "CIE (1931) 2-deg color matching functions", see
    // http://web.archive.org/web/20081228084047/
    // http://www.cvrl.org/database/data/cmfs/ciexyz31.txt
    constexpr double CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[380] = {
        360, 0.000129900000, 0.000003917000, 0.000606100000,
        365, 0.000232100000, 0.000006965000, 0.001086000000,
        370, 0.000414900000, 0.000012390000, 0.001946000000,
        375, 0.000741600000, 0.000022020000, 0.003486000000,
        380, 0.001368000000, 0.000039000000, 0.006450001000,
        385, 0.002236000000, 0.000064000000, 0.010549990000,
        390, 0.004243000000, 0.000120000000, 0.020050010000,
        395, 0.007650000000, 0.000217000000, 0.036210000000,
        400, 0.014310000000, 0.000396000000, 0.067850010000,
        405, 0.023190000000, 0.000640000000, 0.110200000000,
        410, 0.043510000000, 0.001210000000, 0.207400000000,
        415, 0.077630000000, 0.002180000000, 0.371300000000,
        420, 0.134380000000, 0.004000000000, 0.645600000000,
        425, 0.214770000000, 0.007300000000, 1.039050100000,
        430, 0.283900000000, 0.011600000000, 1.385600000000,
        435, 0.328500000000, 0.016840000000, 1.622960000000,
        440, 0.348280000000, 0.023000000000, 1.747060000000,
        445, 0.348060000000, 0.029800000000, 1.782600000000,
        450, 0.336200000000, 0.038000000000, 1.772110000000,
        455, 0.318700000000, 0.048000000000, 1.744100000000,
        460, 0.290800000000, 0.060000000000, 1.669200000000,
        465, 0.251100000000, 0.073900000000, 1.528100000000,
        470, 0.195360000000, 0.090980000000, 1.287640000000,
        475, 0.142100000000, 0.112600000000, 1.041900000000,
        480, 0.095640000000, 0.139020000000, 0.812950100000,
        485, 0.057950010000, 0.169300000000, 0.616200000000,
        490, 0.032010000000, 0.208020000000, 0.465180000000,
        495, 0.014700000000, 0.258600000000, 0.353300000000,
        500, 0.004900000000, 0.323000000000, 0.272000000000,
        505, 0.002400000000, 0.407300000000, 0.212300000000,
        510, 0.009300000000, 0.503000000000, 0.158200000000,
        515, 0.029100000000, 0.608200000000, 0.111700000000,
        520, 0.063270000000, 0.710000000000, 0.078249990000,
        525, 0.109600000000, 0.793200000000, 0.057250010000,
        530, 0.165500000000, 0.862000000000, 0.042160000000,
        535, 0.225749900000, 0.914850100000, 0.029840000000,
        540, 0.290400000000, 0.954000000000, 0.020300000000,
        545, 0.359700000000, 0.980300000000, 0.013400000000,
        550, 0.433449900000, 0.994950100000, 0.008749999000,
        555, 0.512050100000, 1.000000000000, 0.005749999000,
        560, 0.594500000000, 0.995000000000, 0.003900000000,
        565, 0.678400000000, 0.978600000000, 0.002749999000,
        570, 0.762100000000, 0.952000000000, 0.002100000000,
        575, 0.842500000000, 0.915400000000, 0.001800000000,
        580, 0.916300000000, 0.870000000000, 0.001650001000,
        585, 0.978600000000, 0.816300000000, 0.001400000000,
        590, 1.026300000000, 0.757000000000, 0.001100000000,
        595, 1.056700000000, 0.694900000000, 0.001000000000,
        600, 1.062200000000, 0.631000000000, 0.000800000000,
        605, 1.045600000000, 0.566800000000, 0.000600000000,
        610, 1.002600000000, 0.503000000000, 0.000340000000,
        615, 0.938400000000, 0.441200000000, 0.000240000000,
        620, 0.854449900000, 0.381000000000, 0.000190000000,
        625, 0.751400000000, 0.321000000000, 0.000100000000,
        630, 0.642400000000, 0.265000000000, 0.000049999990,
        635, 0.541900000000, 0.217000000000, 0.000030000000,
        640, 0.447900000000, 0.175000000000, 0.000020000000,
        645, 0.360800000000, 0.138200000000, 0.000010000000,
        650, 0.283500000000, 0.107000000000, 0.000000000000,
        655, 0.218700000000, 0.081600000000, 0.000000000000,
        660, 0.164900000000, 0.061000000000, 0.000000000000,
        665, 0.121200000000, 0.044580000000, 0.000000000000,
        670, 0.087400000000, 0.032000000000, 0.000000000000,
        675, 0.063600000000, 0.023200000000, 0.000000000000,
        680, 0.046770000000, 0.017000000000, 0.000000000000,
        685, 0.032900000000, 0.011920000000, 0.000000000000,
        690, 0.022700000000, 0.008210000000, 0.000000000000,
        695, 0.015840000000, 0.005723000000, 0.000000000000,
        700, 0.011359160000, 0.004102000000, 0.000000000000,
        705, 0.008110916000, 0.002929000000, 0.000000000000,
        710, 0.005790346000, 0.002091000000, 0.000000000000,
        715, 0.004109457000, 0.001484000000, 0.000000000000,
        720, 0.002899327000, 0.001047000000, 0.000000000000,
        725, 0.002049190000, 0.000740000000, 0.000000000000,
        730, 0.001439971000, 0.000520000000, 0.000000000000,
        735, 0.000999949300, 0.000361100000, 0.000000000000,
        740, 0.000690078600, 0.000249200000, 0.000000000000,
        745, 0.000476021300, 0.000171900000, 0.000000000000,
        750, 0.000332301100, 0.000120000000, 0.000000000000,
        755, 0.000234826100, 0.000084800000, 0.000000000000,
        760, 0.000166150500, 0.000060000000, 0.000000000000,
        765, 0.000117413000, 0.000042400000, 0.000000000000,
        770, 0.000083075270, 0.000030000000, 0.000000000000,
        775, 0.000058706520, 0.000021200000, 0.000000000000,
        780, 0.000041509940, 0.000014990000, 0.000000000000,
        785, 0.000029353260, 0.000010600000, 0.000000000000,
        790, 0.000020673830, 0.000007465700, 0.000000000000,
        795, 0.000014559770, 0.000005257800, 0.000000000000,
        800, 0.000010253980, 0.000003702900, 0.000000000000,
        805, 0.000007221456, 0.000002607800, 0.000000000000,
        810, 0.000005085868, 0.000001836600, 0.000000000000,
        815, 0.000003581652, 0.000001293400, 0.000000000000,
        820, 0.000002522525, 0.000000910930, 0.000000000000,
        825, 0.000001776509, 0.000000641530, 0.000000000000,
        830, 0.000001251141, 0.000000451810, 0.000000000000, };

    // The conversion matrix from XYZ to linear sRGB color spaces.
    // Values from https://en.wikipedia.org/wiki/SRGB.
    constexpr double XYZ_TO_SRGB[9] = {
        +3.2406, -1.5372, -0.4986,
        -0.9689, +1.8758, +0.0415,
        +0.0557, -0.2040, +1.0570 };
    constexpr int kLambdaMin = 360;
    constexpr int kLambdaMax = 830;

    double CieColorMatchingFunctionTableValue(double wavelength, int column)
    {
        if (wavelength <= kLambdaMin || wavelength >= kLambdaMax) return 0.0;
        double u = (wavelength - kLambdaMin) / 5.0;
        int row = static_cast<int>(std::floor(u)); u -= row;
        return CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) +
               CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
    }

    double Interpolate(const std::vector<double>& wavelengths,
                       const std::vector<double>& wavelength_function, double wavelength)
    {
        if (wavelength < wavelengths[0]) return wavelength_function[0];
        for (unsigned int i = 0; i < wavelengths.size() - 1; ++i)
        {
            if (wavelength < wavelengths[i + 1])
            {
                double u = (wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
                return wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
            }
        }
        return wavelength_function[wavelength_function.size() - 1];
    }

    void ComputeSpectralRadianceToLuminanceFactors(const std::vector<double>& wavelengths,
                                                   const std::vector<double>& solar_irradiance,
                                                   double lambda_power, double* k_r, double* k_g, double* k_b)
    {
        int dlambda = 1; *k_r = 0.0; *k_g = 0.0; *k_b = 0.0;
        double solar_r = Interpolate(wavelengths, solar_irradiance, kLambdaR);
        double solar_g = Interpolate(wavelengths, solar_irradiance, kLambdaG);
        double solar_b = Interpolate(wavelengths, solar_irradiance, kLambdaB);
        for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda)
        {
            double x_bar = CieColorMatchingFunctionTableValue(lambda, 1);
            double y_bar = CieColorMatchingFunctionTableValue(lambda, 2);
            double z_bar = CieColorMatchingFunctionTableValue(lambda, 3);
            const double* xyz2srgb = XYZ_TO_SRGB;
            double r_bar = xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
            double g_bar = xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
            double b_bar = xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
            double irradiance = Interpolate(wavelengths, solar_irradiance, lambda);
            *k_r += r_bar * irradiance / solar_r * pow(lambda / kLambdaR, lambda_power);
            *k_g += g_bar * irradiance / solar_g * pow(lambda / kLambdaG, lambda_power);
            *k_b += b_bar * irradiance / solar_b * pow(lambda / kLambdaB, lambda_power);
        }
        *k_r *= MAX_LUMINOUS_EFFICACY * dlambda;
        *k_g *= MAX_LUMINOUS_EFFICACY * dlambda;
        *k_b *= MAX_LUMINOUS_EFFICACY * dlambda;
    }

    class DensityProfileLayer
    {
    public:
        DensityProfileLayer() : DensityProfileLayer(0.0, 0.0, 0.0, 0.0, 0.0) {}
        DensityProfileLayer(double width, double exp_term, double exp_scale,
                            double linear_term, double constant_term)
        :   width(width), exp_term(exp_term), exp_scale(exp_scale),
            linear_term(linear_term), constant_term(constant_term) {}
        double width, exp_term, exp_scale, linear_term, constant_term;
    };
}

class AtmosphereModel
{
public:
    std::function<std::string (const osg::Vec3&)> _factory;

    AtmosphereModel(bool use_precomputed_luminance, bool use_constant_solar_spectrum,
                    bool use_ozone, bool use_combined_textures)
    {
        static constexpr double kSolarIrradiance[48] = {
            1.11776, 1.14259, 1.01249, 1.14716, 1.72765, 1.73054, 1.6887, 1.61253,
            1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.8298,
            1.8685, 1.8931, 1.85149, 1.8504, 1.8341, 1.8345, 1.8147, 1.78158, 1.7533,
            1.6965, 1.68194, 1.64654, 1.6048, 1.52143, 1.55622, 1.5113, 1.474, 1.4482,
            1.41018, 1.36775, 1.34188, 1.31429, 1.28303, 1.26758, 1.2367, 1.2082,
            1.18737, 1.14683, 1.12362, 1.1058, 1.07124, 1.04992 };
        static constexpr double kOzoneCrossSection[48] = {
            1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.52e-27,
            8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26, 9.016e-26,
            1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25, 4.266e-25,
            4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.74e-25, 3.215e-25,
            2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26,
            6.566e-26, 5.105e-26, 4.15e-26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26,
            2.534e-26, 1.624e-26, 1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27 };
        static constexpr double kDobsonUnit = 2.687e20;
        static constexpr double kMaxOzoneNumberDensity = 300.0 * kDobsonUnit / 15000.0;
        static constexpr double kConstantSolarIrradiance = 1.5;
        static constexpr double kBottomRadius = 6360000.0, kTopRadius = 6420000.0;
        static constexpr double kRayleigh = 1.24062e-6, kRayleighScaleHeight = 8000.0;
        static constexpr double kMieScaleHeight = 1200.0, kMieAngstromAlpha = 0.0;
        static constexpr double kMieAngstromBeta = 5.328e-3;
        static constexpr double kMieSingleScatteringAlbedo = 0.9;
        static constexpr double kMiePhaseFunctionG = 0.8;
        static constexpr double kGroundAlbedo = 0.1;
        static const double max_sun_zenith_angle = 120.0 / 180.0 * atmosphere::kPi;

        atmosphere::DensityProfileLayer rayleigh_layer(0.0, 1.0, -1.0 / kRayleighScaleHeight, 0.0, 0.0);
        atmosphere::DensityProfileLayer mie_layer(0.0, 1.0, -1.0 / kMieScaleHeight, 0.0, 0.0);
        ozone_density.push_back(atmosphere::DensityProfileLayer(25000.0, 0.0, 0.0, 1.0 / 15000.0, -2.0 / 3.0));
        ozone_density.push_back(atmosphere::DensityProfileLayer(0.0, 0.0, 0.0, -1.0 / 15000.0, 8.0 / 3.0));
        for (int l = atmosphere::kLambdaMin; l <= atmosphere::kLambdaMax; l += 10)
        {
            double lambda = static_cast<double>(l) * 1e-3;  // micro-meters
            double mie = kMieAngstromBeta / kMieScaleHeight * pow(lambda, -kMieAngstromAlpha);
            wavelengths.push_back(l);
            if (use_constant_solar_spectrum)
                solar_irradiance.push_back(kConstantSolarIrradiance);
            else
                solar_irradiance.push_back(kSolarIrradiance[(l - atmosphere::kLambdaMin) / 10]);

            rayleigh_scattering.push_back(kRayleigh * pow(lambda, -4));
            mie_scattering.push_back(mie * kMieSingleScatteringAlbedo);
            mie_extinction.push_back(mie);
            absorption_extinction.push_back(use_ozone ?
                (kMaxOzoneNumberDensity * kOzoneCrossSection[(l - atmosphere::kLambdaMin) / 10]) : 0.0);
            ground_albedo.push_back(kGroundAlbedo);
        }

        Init(atmosphere::kSunAngularRadius, kBottomRadius, kTopRadius,
             { rayleigh_layer }, { mie_layer }, ozone_density, kMiePhaseFunctionG,
             max_sun_zenith_angle, atmosphere::kLengthUnitInMeters,
             use_precomputed_luminance ? 15 : 3, use_combined_textures);
    }

protected:
    void Init(double sun_angular_radius, double bottom_radius, double top_radius,
              const std::vector<atmosphere::DensityProfileLayer>& rayleigh_density,
              const std::vector<atmosphere::DensityProfileLayer>& mie_density,
              const std::vector<atmosphere::DensityProfileLayer>& absorption_density,
              double mie_phase_function_g, double max_sun_zenith_angle, double length_unit_in_meters,
              unsigned int num_precomputed_wavelengths, bool combine_scattering_textures)
    {
        auto vec_to_string = [this](const std::vector<double>& v, const osg::Vec3& lambdas, double scale)
            {
                double r = atmosphere::Interpolate(wavelengths, v, lambdas[0]) * scale;
                double g = atmosphere::Interpolate(wavelengths, v, lambdas[1]) * scale;
                double b = atmosphere::Interpolate(wavelengths, v, lambdas[2]) * scale;
                return "vec3(" + std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b) + ")";
            };
        auto density_layer = [length_unit_in_meters](const atmosphere::DensityProfileLayer& layer)
            {
                return "DensityProfileLayer(" +
                    std::to_string(layer.width / length_unit_in_meters) + "," +
                    std::to_string(layer.exp_term) + "," +
                    std::to_string(layer.exp_scale * length_unit_in_meters) + "," +
                    std::to_string(layer.linear_term * length_unit_in_meters) + "," +
                    std::to_string(layer.constant_term) + ")";
            };
        auto density_profile = [density_layer](std::vector<atmosphere::DensityProfileLayer> layers)
            {
                constexpr int kLayerCount = 2;
                while (layers.size() < kLayerCount) layers.insert(layers.begin(), atmosphere::DensityProfileLayer());
                std::string result = "DensityProfile(DensityProfileLayer[" + std::to_string(kLayerCount) + "](";
                for (int i = 0; i < kLayerCount; ++i)
                    { result += density_layer(layers[i]); result += i < kLayerCount - 1 ? "," : "))"; }
                return result;
            };

        bool precompute_illuminance = (num_precomputed_wavelengths > 3);
        double sky_k_r, sky_k_g, sky_k_b, sun_k_r, sun_k_g, sun_k_b;
        if (precompute_illuminance)
            sky_k_r = sky_k_g = sky_k_b = atmosphere::MAX_LUMINOUS_EFFICACY;
        else
            atmosphere::ComputeSpectralRadianceToLuminanceFactors(
                wavelengths, solar_irradiance, -3 /* lambda_power */, &sky_k_r, &sky_k_g, &sky_k_b);
        atmosphere::ComputeSpectralRadianceToLuminanceFactors(
            wavelengths, solar_irradiance, 0 /* lambda_power */, &sun_k_r, &sun_k_g, &sun_k_b);
        _factory = [=](const osg::Vec3& lambdas)
            {
                return std::string(combine_scattering_textures ? "#define COMBINED_SCATTERING_TEXTURES\n" : "") +
                       std::string(precompute_illuminance ? "" : "#define RADIANCE_API_ENABLED\n") +
                       "#include \"atmospheric_scattering.module.glsl\"\n" +
                    "const AtmosphereParameters ATMOSPHERE = AtmosphereParameters(\n" +
                        vec_to_string(solar_irradiance, lambdas, 1.0) + ",\n" +
                        std::to_string(sun_angular_radius) + ",\n" +
                        std::to_string(bottom_radius / length_unit_in_meters) + ",\n" +
                        std::to_string(top_radius / length_unit_in_meters) + ",\n" +
                        density_profile(rayleigh_density) + ",\n" +
                        vec_to_string(rayleigh_scattering, lambdas, length_unit_in_meters) + ",\n" +
                        density_profile(mie_density) + ",\n" +
                        vec_to_string(mie_scattering, lambdas, length_unit_in_meters) + ",\n" +
                        vec_to_string(mie_extinction, lambdas, length_unit_in_meters) + ",\n" +
                        std::to_string(mie_phase_function_g) + ",\n" +
                        density_profile(absorption_density) + ",\n" +
                        vec_to_string(absorption_extinction, lambdas, length_unit_in_meters) + ",\n" +
                        vec_to_string(ground_albedo, lambdas, 1.0) + ",\n" +
                        std::to_string(cos(max_sun_zenith_angle)) + ");\n" +
                    "const vec3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(" +
                        std::to_string(sky_k_r) + "," + std::to_string(sky_k_g) + "," + std::to_string(sky_k_b) + ");\n" +
                    "const vec3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(" +
                        std::to_string(sun_k_r) + "," + std::to_string(sun_k_g) + "," + std::to_string(sun_k_b) + ");\n";
            };
    }

    std::vector<atmosphere::DensityProfileLayer> ozone_density;
    std::vector<double> wavelengths, solar_irradiance, rayleigh_scattering;
    std::vector<double> mie_scattering, mie_extinction, absorption_extinction, ground_albedo;
};

osg::Camera* createRTTCameraForImage(osg::Camera::BufferComponent buffer, osg::Image* image, bool screenSpaced)
{
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setRenderOrder(osg::Camera::PRE_RENDER);
    if (image)
    {
        camera->setViewport(0, 0, image->s(), image->t());
        camera->attach(buffer, image);
    }

    if (screenSpaced)
    {
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
        camera->setViewMatrix(osg::Matrix::identity());
        camera->addChild(osgVerse::createScreenQuad(
            osg::Vec3(), 1.0f, 1.0f, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f)));
    }
    return camera.release();
}

const char* kGeometryShader = {
    "#extension GL_EXT_geometry_shader4 : enable\n"
    "uniform int layer;\n"
    "void main() {\n"
    "    gl_Position = VERSE_GS_POS(0);\n"
    "    gl_Layer = layer;\nEmitVertex();\n"
    "    gl_Position = VERSE_GS_POS(1);\n"
    "    gl_Layer = layer;\nEmitVertex();\n"
    "    gl_Position = VERSE_GS_POS(2);\n"
    "    gl_Layer = layer;\nEmitVertex();\n"
    "    EndPrimitive();\n"
    "}\n"
};

const char* kComputeTransmittanceShader = {
    "VERSE_FS_OUT vec4 transmittance;\n"
    "void main() {\n"
    "    transmittance.xyz = ComputeTransmittanceToTopAtmosphereBoundaryTexture(ATMOSPHERE, gl_FragCoord.xy);\n"
    "    transmittance.w = 1.0; VERSE_FS_FINAL(transmittance);\n"
    "}\n"
};

const char* kComputeDirectIrradianceShader = {
    "uniform sampler2D transmittance;\n"
    "#ifdef VERSE_GLES3\n"
    "    layout(location = 0) VERSE_FS_OUT vec3 delta_irradiance;\n"
    "    layout(location = 1) VERSE_FS_OUT vec3 irradiance;\n"
    "#endif\n"
    "void main() {\n"
    "    vec3 delta = ComputeDirectIrradianceTexture(ATMOSPHERE, transmittance, gl_FragCoord.xy);\n"
    "#ifdef VERSE_GLES3\n"
    "    delta_irradiance = delta; irradiance = vec3(0.0);\n"
    "#else\n"
    "    gl_FragData[0] = vec4(delta, 1.0); gl_FragData[1] = vec4(vec3(0.0), 1.0);\n"
    "#endif\n"
    "}\n"
};

const char* kComputeSingleScatteringShader = {
    "uniform sampler2D transmittance;\n"
    "uniform mat3 luminance_from_radiance;\n"
    "uniform int layer;\n"
    "#ifdef VERSE_GLES3\n"
    "    layout(location = 0) out vec3 delta_rayleigh;\n"
    "    layout(location = 1) out vec3 delta_mie;\n"
    "    layout(location = 2) out vec4 scattering;\n"
    "    layout(location = 3) out vec3 single_mie_scattering;\n"
    "#endif\n"
    "void main() {\n"
    "    vec3 d_rayleigh, d_mie, single_mie; vec4 s0;\n"
    "    ComputeSingleScatteringTexture(ATMOSPHERE, transmittance, vec3(gl_FragCoord.xy, layer + 0.5),\n"
    "                                   d_rayleigh, d_mie);\n"
    "    s0 = vec4(luminance_from_radiance * d_rayleigh.rgb, (luminance_from_radiance * d_mie).r);\n"
    "    single_mie = luminance_from_radiance * d_mie;"
    "#ifdef VERSE_GLES3\n"
    "    delta_rayleigh = d_rayleigh; delta_mie = d_mie; scattering = s0; single_mie_scattering = single_mie;\n"
    "#else\n"
    "    gl_FragData[0] = vec4(d_rayleigh, 1.0); gl_FragData[1] = vec4(d_mie, 1.0);\n"
    "    gl_FragData[2] = s0; gl_FragData[3] = single_mie;\n"
    "#endif\n"
    "}\n"
};

const char* kComputeScatteringDensityShader = {
    "uniform sampler2D transmittance, irradiance; \n"
    "uniform sampler3D single_rayleigh_scattering;\n"
    "uniform sampler3D single_mie_scattering;\n"
    "uniform sampler3D multiple_scattering;\n"
    "uniform int scattering_order, layer;\n"
    "VERSE_FS_OUT vec4 scattering_density;\n"
    "void main() {\n"
    "    scattering_density.xyz = ComputeScatteringDensityTexture(\n"
    "        ATMOSPHERE, transmittance, single_rayleigh_scattering, single_mie_scattering, multiple_scattering,\n"
    "        irradiance, vec3(gl_FragCoord.xy, layer + 0.5), scattering_order);\n"
    "    scattering_density.w = 1.0; VERSE_FS_FINAL(scattering_density);\n"
    "}\n"
};

const char* kComputeIndirectIrradianceShader = {
    "uniform sampler3D single_rayleigh_scattering;\n"
    "uniform sampler3D single_mie_scattering;\n"
    "uniform sampler3D multiple_scattering;\n"
    "uniform mat3 luminance_from_radiance;\n"
    "uniform int scattering_order;\n"
    "#ifdef VERSE_GLES3\n"
    "    layout(location = 0) out vec3 delta_irradiance;\n"
    "    layout(location = 1) out vec3 irradiance;\n"
    "#endif\n"
    "void main() {\n"
    "    vec3 d_irradiance = ComputeIndirectIrradianceTexture(\n"
    "        ATMOSPHERE, single_rayleigh_scattering single_mie_scattering, multiple_scattering,\n"
    "        gl_FragCoord.xy, scattering_order);\n"
    "    vec3 irr = luminance_from_radiance * d_irradiance;\n"
    "#ifdef VERSE_GLES3\n"
    "    delta_irradiance = d_irradiance; irradiance = irr;\n"
    "#else\n"
    "    gl_FragData[0] = vec4(d_irradiance, 1.0); gl_FragData[1] = vec4(irr, 1.0);\n"
    "#endif\n"
    "}\n"
};

const char* kComputeMultipleScatteringShader = {
    "uniform sampler2D transmittance;\n"
    "uniform sampler3D scattering_density;\n"
    "uniform mat3 luminance_from_radiance;\n"
    "uniform int layer;\n"
    "#ifdef VERSE_GLES3\n"
    "    layout(location = 0) out vec3 delta_multiple_scattering;\n"
    "    layout(location = 1) out vec4 scattering;\n"
    "#endif\n"
    "void main() {\n"
    "    float nu = 0.0; vec3 d_multiple_scattering;\n"
    "    d_multiple_scattering = ComputeMultipleScatteringTexture(\n"
    "        ATMOSPHERE, transmittance, scattering_density, vec3(gl_FragCoord.xy, layer + 0.5), nu);\n"
    "    vec4 s0 = vec4(luminance_from_radiance * d_multiple_scattering.rgb / RayleighPhaseFunction(nu), 0.0);\n"
    "#ifdef VERSE_GLES3\n"
    "    delta_multiple_scattering = d_multiple_scattering; scattering = s0;\n"
    "#else\n"
    "    gl_FragData[0] = vec4(d_multiple_scattering, 1.0); gl_FragData[1] = s0;\n"
    "#endif\n"
    "}\n"
};

constexpr int TRANSMITTANCE_TEXTURE_WIDTH = 256;
constexpr int TRANSMITTANCE_TEXTURE_HEIGHT = 64;
constexpr int IRRADIANCE_TEXTURE_WIDTH = 64;
constexpr int IRRADIANCE_TEXTURE_HEIGHT = 16;
constexpr int SCATTERING_TEXTURE_R_SIZE = 32;
constexpr int SCATTERING_TEXTURE_MU_SIZE = 128;
constexpr int SCATTERING_TEXTURE_MU_S_SIZE = 32;
constexpr int SCATTERING_TEXTURE_NU_SIZE = 8;
constexpr int SCATTERING_TEXTURE_WIDTH = SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
constexpr int SCATTERING_TEXTURE_HEIGHT = SCATTERING_TEXTURE_MU_SIZE;
constexpr int SCATTERING_TEXTURE_DEPTH = SCATTERING_TEXTURE_R_SIZE;

int main(int argc, char** argv)
{
    bool use_precomputed_luminance = false;
    bool use_constant_solar_spectrum = false;
    bool use_ozone = true, use_combined_textures = true;
    AtmosphereModel model(use_precomputed_luminance, use_constant_solar_spectrum, use_ozone, use_combined_textures);

    osg::ref_ptr<osg::Image> transmittance = new osg::Image;
    transmittance->allocateImage(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, 1, GL_RGBA, GL_FLOAT);
    transmittance->setInternalTextureFormat(GL_RGBA32F_ARB);

    std::string header = model._factory(osg::Vec3(atmosphere::kLambdaR, atmosphere::kLambdaG, atmosphere::kLambdaB));
    osg::Shader* vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "std_common_quad.vert.glsl");
    osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, header + kComputeTransmittanceShader);

    int cxtVer = 0, glslVer = 0; osgVerse::guessOpenGLVersions(cxtVer, glslVer);
    osgVerse::Pipeline::createShaderDefinitions(vs, cxtVer, glslVer);
    osgVerse::Pipeline::createShaderDefinitions(fs, cxtVer, glslVer);

    osg::ref_ptr<osg::Program> prog = new osg::Program;
    prog->addShader(vs); prog->addShader(fs);
    osg::Camera* cam = createRTTCameraForImage(osg::Camera::COLOR_BUFFER0, transmittance.get(), true);
    cam->getOrCreateStateSet()->setAttributeAndModes(
        prog.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // Scene graph
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(cam);

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    for (int i = 0; i < 3; ++i) viewer.frame();

    osgDB::writeImageFile(*transmittance, "atmo_transmittance.tif");
    //osg::ref_ptr<osg::ImageSequence> seq = new osg::ImageSequence;
    //seq->addImage(transmittance.get());
    //if (osgDB::writeImageFile(*seq, "atmo_scattering.rseq.verse_image"))
    //    std::cout << "Atmosphere textures outputted" << "\n";
    return 0;
}
