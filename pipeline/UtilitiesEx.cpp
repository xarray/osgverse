#include <osg/io_utils>
#include <osg/Version>
#include <osg/ComputeBoundsVisitor>
#include <osg/FrameBufferObject>
#include <osg/RenderInfo>
#include <osg/GLExtensions>
#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osg/PolygonMode>
#include <osg/Geode>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/ConvertUTF>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>
#include <chrono>
#include <codecvt>
#include <iostream>
#include <array>
#include <random>

#include "modeling/Math.h"
#include "ShaderLibrary.h"
#include "Pipeline.h"
#include "Utilities.h"

#define srnd() (2 * frandom(&seed) - 1)
using namespace osgVerse;

static long lrandom(long* seed)
{
    *seed = (*seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return *seed;
}

static float frandom(long* seed)
{ long r = lrandom(seed) >> (31 - 24); return r / (float)(1 << 24); }

static float grandom(float mean, float stdDeviation, long* seed)
{
    float x1, x2, w, y1;
    static float y2;
    static int use_last = 0;
    if (use_last) { y1 = y2; use_last = 0; }
    else
    {
        do {
            x1 = 2.0f * frandom(seed) - 1.0f;
            x2 = 2.0f * frandom(seed) - 1.0f;
            w = x1 * x1 + x2 * x2;
        } while (w >= 1.0f);
        w = sqrt((-2.0f * log(w)) / w);
        y1 = x1 * w; y2 = x2 * w; use_last = 1;
    }
    return mean + y1 * stdDeviation;
}

static unsigned char* loadAllData(const std::string& file, unsigned int& size, unsigned int offset)
{
    std::ifstream ifs(file.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if (!ifs) return NULL;

    size = (int)ifs.tellg() - offset;
    ifs.seekg(offset, std::ios::beg); ifs.clear();

    unsigned char* imageData = new unsigned char[size];
    ifs.read((char*)imageData, size); ifs.close();
    return imageData;
}

void EarthAtmosphereOcean::applyToGlobe(osg::StateSet* ss, osg::Texture* baseTex, osg::Texture* maskTex,
                                        osg::Texture* extraTex, osg::Shader* vs, osg::Shader* fs, Pipeline* ref)
{
    if (!ss || !vs || !fs) return;
    vs->setName("Scattering_Globe_VS"); fs->setName("Scattering_Globe_FS");
    if (!ref)
    {
        Pipeline::createShaderDefinitions(vs, 100, 130);
        Pipeline::createShaderDefinitions(fs, 100, 130);
    }

    ss->setTextureAttributeAndModes(0, baseTex);
    ss->setTextureAttributeAndModes(1, maskTex);
    ss->setTextureAttributeAndModes(2, extraTex);
    ss->getOrCreateUniform("SceneSampler", osg::Uniform::INT)->set((int)0);
    ss->getOrCreateUniform("MaskSampler", osg::Uniform::INT)->set((int)1);
    ss->getOrCreateUniform("ExtraLayerSampler", osg::Uniform::INT)->set((int)2);
    ss->getOrCreateUniform("UvOffset1", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    ss->getOrCreateUniform("UvOffset2", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    ss->getOrCreateUniform("UvOffset3", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));

    osg::Program* program = apply(ss, vs, fs, 3, ref);
    program->addBindAttribLocation("osg_GlobeData", 1);  // for computing ocean plane
}

void EarthAtmosphereOcean::applyToOcean(osg::StateSet* ss, osg::Texture* postMaskTex, osg::Texture* waveTex,
                                        osg::Shader* vs, osg::Shader* fs, Pipeline* ref)
{
    if (!ss || !vs || !fs) return;
    vs->setName("Global_Ocean_VS"); fs->setName("Global_Ocean_FS");
    if (!ref)
    {
        Pipeline::createShaderDefinitions(vs, 100, 130);
        Pipeline::createShaderDefinitions(fs, 100, 130);
    }

    ss->setTextureAttributeAndModes(0, postMaskTex);
    ss->setTextureAttributeAndModes(1, waveTex);
    ss->getOrCreateUniform("EarthMaskSampler", osg::Uniform::INT)->set((int)0);
    ss->getOrCreateUniform("WavesSampler", osg::Uniform::INT)->set((int)1);
    apply(ss, vs, fs, 2, ref);
    for (std::map<std::string, osg::ref_ptr<osg::Uniform>>::iterator it = oceanUniforms.begin();
         it != oceanUniforms.end(); ++it) ss->addUniform(it->second.get());
}

void EarthAtmosphereOcean::applyToAtmosphere(osg::StateSet* ss, osg::Texture* earthSceneTex,
                                             osg::Shader* vs, osg::Shader* fs, Pipeline* ref)
{
    if (!ss || !vs || !fs) return;
    vs->setName("Scattering_Sky_VS"); fs->setName("Scattering_Sky_FS");
    if (!ref)
    {
        Pipeline::createShaderDefinitions(vs, 100, 130);
        Pipeline::createShaderDefinitions(fs, 100, 130);
    }

    ss->setTextureAttributeAndModes(0, earthSceneTex);
    ss->getOrCreateUniform("SceneSampler", osg::Uniform::INT)->set((int)0);
    apply(ss, vs, fs, 1, ref);
}

osg::Program* EarthAtmosphereOcean::apply(osg::StateSet* ss, osg::Shader* vs, osg::Shader* fs,
                                          unsigned int startU, Pipeline* refPipeline)
{
    if (!ss || !vs || !fs) return NULL;
    if (refPipeline)
    {
        refPipeline->createShaderDefinitionsFromPipeline(vs);
        refPipeline->createShaderDefinitionsFromPipeline(fs);
    }

    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(vs); program->addShader(fs);
    ss->setAttributeAndModes(program.get());
    ss->setTextureAttributeAndModes(startU + 0, transmittance.get());
    ss->setTextureAttributeAndModes(startU + 1, irradiance.get());
    ss->setTextureAttributeAndModes(startU + 2, inscatter.get());
    ss->setTextureAttributeAndModes(startU + 3, glare.get());
    ss->getOrCreateUniform("TransmittanceSampler", osg::Uniform::INT)->set((int)startU + 0);
    ss->getOrCreateUniform("SkyIrradianceSampler", osg::Uniform::INT)->set((int)startU + 1);
    ss->getOrCreateUniform("InscatterSampler", osg::Uniform::INT)->set((int)startU + 2);
    ss->getOrCreateUniform("GlareSampler", osg::Uniform::INT)->set((int)startU + 3);
    for (std::map<std::string, osg::ref_ptr<osg::Uniform>>::iterator it = commonUniforms.begin();
         it != commonUniforms.end(); ++it) ss->addUniform(it->second.get());
    return program.get();
}

void EarthAtmosphereOcean::update(osg::Camera* camera)
{
    osg::Matrix invViewMatrix = camera->getInverseViewMatrix();
    osg::Matrix projMatrix = camera->getProjectionMatrix();
    osg::Vec3d worldCam = invViewMatrix.getTrans();
    osg::Vec3d worldLLA = Coordinate::convertECEFtoLLA(worldCam);
    commonUniforms["CameraToWorld"]->set(osg::Matrixf(invViewMatrix));
    commonUniforms["CameraToScreen"]->set(osg::Matrixf(projMatrix));
    commonUniforms["ScreenToCamera"]->set(osg::Matrixf::inverse(projMatrix));
    commonUniforms["WorldCameraPos"]->set(osg::Vec3(worldCam));
    commonUniforms["WorldCameraLLA"]->set(osg::Vec3(worldLLA));

    float width = camera->getViewport() ? camera->getViewport()->width() : 1920.0f;
    float height = camera->getViewport() ? camera->getViewport()->height() : 1080.0f;
    commonUniforms["ScreenSize"]->set(osg::Vec2(width, height));
}

void EarthAtmosphereOcean::updateOcean(osg::Camera* camera)
{
    osg::Matrix ctol = camera->getInverseViewMatrix();
    osg::Vec3d cl = osg::Vec3() * ctol;
    double radius = osg::WGS_84_RADIUS_EQUATOR;
    if ((radius == 0.0 && cl.z() > oceanMinZ) || (radius > 0.0 && cl.length() > radius + oceanMinZ))
    { oceanFromLocal = osg::Matrix(); oceanOffset = osg::Vec3(); }

    osg::Vec3d ux, uy, uz, oo;
    if (radius == 0.0)
    {
        ux = osg::X_AXIS; uy = osg::Y_AXIS; uz = osg::Z_AXIS;
        oo = osg::Vec3d(cl.x(), cl.y(), 0.0);
    }
    else
    {   // spherical ocean
        uz = cl; uz.normalize(); // unit z vector of ocean frame, in local space
        if (!oceanFromLocal.isIdentity())
        {
            ux = osg::Vec3d(oceanFromLocal(1, 0), oceanFromLocal(1, 1), oceanFromLocal(1, 2));
            ux = (ux ^ uz); ux.normalize();
        }
        else
            { ux = osg::Z_AXIS; ux = (ux ^ uz); ux.normalize(); }
        uy = (uz ^ ux); // unit y vector
        oo = uz * radius; // origin of ocean frame, in local space
    }

    osg::Matrix ltoo(ux.x(), uy.x(), uz.x(), 0.0,
                     ux.y(), uy.y(), uz.y(), 0.0,
                     ux.z(), uy.z(), uz.z(), 0.0,
                     -(ux * oo), -(uy * oo), -(uz * oo), 1.0);
    osg::Matrix ctoo = ctol * ltoo;  // compute ctoo = CameraToOcean transform
    if (!oceanFromLocal.isIdentity())
    {
        osg::Vec3d delta = osg::Vec3d() * osg::Matrix::inverse(oceanFromLocal) * ltoo;
        oceanOffset += delta;
    }
    oceanFromLocal = ltoo;

    // Compute ocean-world-camera matrices
    osg::Matrix ctos = camera->getProjectionMatrix();
    osg::Matrix stoc = osg::Matrix::inverse(ctos);
    osg::Matrix otoc = osg::Matrix::inverse(ctoo);
    otoc.setTrans(osg::Vec3());

    osg::Vec3 wDir, oDir;
    osg::Uniform* worldSunDir = commonUniforms["WorldSunDir"].get();
    osg::Uniform* oceanSunDir = oceanUniforms["OceanSunDir"].get();
    worldSunDir->get(wDir); oceanSunDir->set(osg::Matrixf::transform3x3(wDir, ltoo));

    osg::Vec3d oc = osg::Vec3d() * ctoo; float h = oc.z();  // if h < 0, we are under the ocean...
    commonUniforms["UnderOcean"]->set(h);
    oceanUniforms["CameraToOcean"]->set(osg::Matrixf(ctoo));
    oceanUniforms["OceanToCamera"]->set(osg::Matrixf(otoc));
    oceanUniforms["OceanToWorld"]->set(osg::Matrixf::inverse(ltoo));
    oceanUniforms["OceanCameraPos"]->set(osg::Vec3(-oceanOffset.x(), -oceanOffset.y(), h));

    // Compute horizon1 & horizon2
    osg::Vec4d temp;
    temp = osg::Vec4d(0.0, 0.0, 0.0, 1.0) * stoc;
    temp = osg::Vec4d(temp[0], temp[1], temp[2], 0.0) * ctoo;
    osg::Vec3d A0(temp[0], temp[1], temp[2]);
    temp = osg::Vec4d(1.0, 0.0, 0.0, 0.0) * stoc;
    temp = osg::Vec4d(temp[0], temp[1], temp[2], 0.0) * ctoo;
    osg::Vec3d dA(temp[0], temp[1], temp[2]);
    temp = osg::Vec4d(0.0, 1.0, 0.0, 0.0) * stoc;
    temp = osg::Vec4d(temp[0], temp[1], temp[2], 0.0) * ctoo;
    osg::Vec3d B(temp[0], temp[1], temp[2]);

    if (radius == 0.0)
    {
        oceanUniforms["Horizon1"]->set(osg::Vec3(-(h * 1e-6 + A0.z()) / B.z(), -dA.z() / B.z(), 0.0));
        oceanUniforms["Horizon2"]->set(osg::Vec3());
    }
    else
    {
        double h1 = h * (h + 2.0 * radius), h2 = (h + radius) * (h + radius);
        double alpha = (B * B) * h1 - B.z() * B.z() * h2;
        alpha = -fabs(alpha);  // FIXME: modified from proland to keep ocean always seen, can it work?

        double beta0 = ((A0 * B) * h1 - B.z() * A0.z() * h2) / alpha;
        double beta1 = ((dA * B) * h1 - B.z() * dA.z() * h2) / alpha;
        double gamma0 = ((A0 * A0) * h1 - A0.z() * A0.z() * h2) / alpha;
        double gamma1 = ((A0 * dA) * h1 - A0.z() * dA.z() * h2) / alpha;
        double gamma2 = ((dA * dA) * h1 - dA.z() * dA.z() * h2) / alpha;
        oceanUniforms["Horizon1"]->set(osg::Vec3(-beta0, -beta1, alpha));
        oceanUniforms["Horizon2"]->set(osg::Vec3(
            beta0 * beta0 - gamma0, 2.0 * (beta0 * beta1 - gamma1), beta1 * beta1 - gamma2));
    }

    // angle under which a screen pixel is viewed from the camera
    float fov = 0.0f, aspectRatio = 0.0f, znear = 0.0f, zfar = 0.0f;
    float width = camera->getViewport() ? camera->getViewport()->width() : 1920.0f;
    float height = camera->getViewport() ? camera->getViewport()->height() : 1080.0f;
    camera->getProjectionMatrix().getPerspective(fov, aspectRatio, znear, zfar);
    commonUniforms["ScreenSize"]->set(osg::Vec2(width, height));

    // FIXME: pixelSize affects wave tiling, should be treated carefully
    float pixelSize = atan(tan(osg::inDegrees(fov * aspectRatio * (float)oceanPixelScale)) / (height / 2.0f));
    float k = log(oceanLambdaAndHeight[1]) / log(2.0f) - log(oceanLambdaAndHeight[0]) / log(2.0f);
    oceanUniforms["HeightOffset"]->set(-oceanLambdaAndHeight[2]);
    oceanUniforms["SeaGridLODs"]->set(osg::Vec4(
        oceanGridResolution, pixelSize * oceanGridResolution,
        log(oceanLambdaAndHeight[0]) / log(2.0f), (oceanWaveCount - 1.0f) / k));
}

bool EarthAtmosphereOcean::create(const std::string& tr, const std::string& ir,
                                  const std::string& gl, const std::string& in)
{
    unsigned int trSize = 0, irSize = 0, inSize = 0;
    unsigned char *trData = loadAllData(tr, trSize, 0), *irData = loadAllData(ir, irSize, 0);
    unsigned char *inData = loadAllData(in, inSize, 0);
    if (!trData || !irData || !inData) return false;

    osg::ref_ptr<osg::Texture> trTex = rawFloatingTexture2D(trData, 256, 64, true);
    osg::ref_ptr<osg::Texture> irTex = rawFloatingTexture2D(irData, 64, 16, true);
    osg::ref_ptr<osg::Texture> inTex = rawFloatingTexture3D(inData, 256, 128, 32, false);
    osg::ref_ptr<osg::Texture> glTex = createTexture2D(osgDB::readImageFile(gl), osg::Texture::CLAMP);
    return create(trTex.get(), irTex.get(), glTex.get(), inTex.get());
}

bool EarthAtmosphereOcean::create(osg::Texture* tr, osg::Texture* ir, osg::Texture* gl, osg::Texture* in)
{
    transmittance = tr; irradiance = ir; glare = gl; inscatter = in;
    commonUniforms["CameraToWorld"] = new osg::Uniform("CameraToWorld", osg::Matrixf());
    commonUniforms["CameraToScreen"] = new osg::Uniform("CameraToScreen", osg::Matrixf());
    commonUniforms["ScreenToCamera"] = new osg::Uniform("ScreenToCamera", osg::Matrixf());
    commonUniforms["WorldSunDir"] = new osg::Uniform("WorldSunDir", osg::Vec3(-1.0f, 0.0f, 0.0f));
    commonUniforms["WorldCameraPos"] = new osg::Uniform("WorldCameraPos", osg::Vec3(0.0f, 0.0f, 0.0f));
    commonUniforms["WorldCameraLLA"] = new osg::Uniform("WorldCameraLLA", osg::Vec3(0.0f, 0.0f, 0.0f));
    commonUniforms["EarthOrigin"] = new osg::Uniform("EarthOrigin", osg::Vec3(0.0f, 0.0f, 0.0f));
    commonUniforms["ScreenSize"] = new osg::Uniform("ScreenSize", osg::Vec2(1920.0f, 1080.0f));
    commonUniforms["GlobalOpaque"] = new osg::Uniform("GlobalOpaque", 1.0f);
    commonUniforms["OceanOpaque"] = new osg::Uniform("OceanOpaque", 1.0f);
    commonUniforms["UnderOcean"] = new osg::Uniform("UnderOcean", 1.0f);
    commonUniforms["HdrExposure"] = new osg::Uniform("HdrExposure", 0.25f);

    oceanUniforms["CameraToOcean"] = new osg::Uniform("CameraToOcean", osg::Matrixf());
    oceanUniforms["OceanToCamera"] = new osg::Uniform("OceanToCamera", osg::Matrixf());
    oceanUniforms["OceanToWorld"] = new osg::Uniform("OceanToWorld", osg::Matrixf());
    oceanUniforms["SeaGridLODs"] = new osg::Uniform("SeaGridLODs", osg::Vec4());
    oceanUniforms["SeaColor"] = new osg::Uniform("SeaColor", osg::Vec3(1.f / 255.f, 4.f / 255.f, 12.f / 255.f));
    oceanUniforms["OceanCameraPos"] = new osg::Uniform("OceanCameraPos", osg::Vec3());
    oceanUniforms["OceanSunDir"] = new osg::Uniform("OceanSunDir", osg::Vec3());
    oceanUniforms["Horizon1"] = new osg::Uniform("Horizon1", osg::Vec3());
    oceanUniforms["Horizon2"] = new osg::Uniform("Horizon2", osg::Vec3());
    oceanUniforms["Radius"] = new osg::Uniform("Radius", (float)osg::WGS_84_RADIUS_EQUATOR);
    oceanUniforms["WaveCount"] = new osg::Uniform("WaveCount", (float)oceanWaveCount);
    oceanUniforms["HeightOffset"] = new osg::Uniform("HeightOffset", 0.0f);
    oceanUniforms["SeaRoughness"] = new osg::Uniform("SeaRoughness", 0.0f);
    return true;
}

osg::Geometry* EarthAtmosphereOcean::createOceanGrid(int width, int height)
{
    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);

    float f = 1.25f;
    int NX = int(f * width / oceanGridResolution);
    int NY = int(f * height / oceanGridResolution);
    for (int i = 0; i < NY; ++i)
    {
        for (int j = 0; j < NX; ++j)
            va->push_back(osg::Vec3(2.0 * f * j / ((float)NX - 1.0f) - f,
                                    2.0 * f * i / ((float)NY - 1.0f) - f, 0.0f));
    }

    for (int i = 0; i < NY - 1; ++i)
    {
        for (int j = 0; j < NX - 1; ++j)
        {
            de->push_back(i * NX + j); de->push_back(i * NX + j + 1); de->push_back((i + 1) * NX + j);
            de->push_back((i + 1) * NX + j); de->push_back(i * NX + j + 1); de->push_back((i + 1) * NX + j + 1);
        }
    }

    osg::Geometry* geom = new osg::Geometry;
    geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va); geom->addPrimitiveSet(de); return geom;
}

osg::Texture* EarthAtmosphereOcean::createOceanWaves(float& seaRoughness)
{
    long seed = 1234567;
    float min = log(oceanLambdaAndHeight[0]) / log(2.0f);
    float max = log(oceanLambdaAndHeight[1]) / log(2.0f);

    std::vector<osg::Vec4> waves(oceanWaveCount);
    float sigmaXsq = 0.0f, sigmaYsq = 0.0f;
    float heightVariance = 0.0f, amplitudeMax = 0.0f;
    oceanLambdaAndHeight[2] = 0.0f;

#define nbAngles  5 // impair
#define angle(i)  (1.5 * (((i) % nbAngles) / (float)(nbAngles / 2) - 1))
#define dangle()  (1.5 / (float)(nbAngles / 2))
    float s = 0.0f;
    float Wa[nbAngles];  // normalised gaussian samples
    int index[nbAngles]; // to hash angle order
    for (int i = 0; i < nbAngles; i++)
    {
        index[i] = i; float a = angle(i); // (i/(float)(nbAngle/2)-1)*1.5;
        s += Wa[i] = exp(-.5f * a * a);
    }
    for (int i = 0; i < nbAngles; i++) Wa[i] /= s;

    const float waveDispersion = 0.9f, U0 = 10.0f;
    const int spectrumType = 2;
    for (int i = 0; i < oceanWaveCount; ++i)
    {
        float x = i / float(oceanWaveCount - 1.0f);
        float lambda = pow(2.0f, (1.0f - x) * min + x * max);
        float ktheta = grandom(0.0f, 1.0f, &seed) * waveDispersion;
        float knorm = 2.0f * osg::PI / lambda;
        float omega = sqrt(9.81f * knorm);
        float amplitude = 0.0f;
        if (spectrumType == 1)
            amplitude = oceanLambdaAndHeight[3] * grandom(0.5f, 0.15f, &seed)
                      / (knorm * oceanLambdaAndHeight[1] / (2.0f * osg::PI));
        else if (spectrumType == 2)
        {
            float step = (max - min) / (oceanWaveCount - 1); // dlambda/di
            float omega0 = 9.81f / U0; // 100.0;
            if ((i % (nbAngles)) == 0)
            {   // scramble angle ordre
                for (int k = 0; k < nbAngles; k++)
                {   // do N swap in indices
                    int n1 = lrandom(&seed) % nbAngles, n2 = lrandom(&seed) % nbAngles, n;
                    n = index[n1]; index[n1] = index[n2]; index[n2] = n;
                }
            }
            ktheta = waveDispersion * (angle(index[(i) % nbAngles]) + .4 * srnd() * dangle());
            ktheta *= 1 / (1 + 40 * pow(omega0 / omega, 4));
            amplitude = (8.1e-3 * 9.81f * 9.81f) / pow(omega, 5.0f) * exp(-0.74f * pow(omega0 / omega, 4.0f));
            amplitude *= .5 * sqrt(2.0f * 3.14f * 9.81f / lambda) * nbAngles * step; // (2/step-step/2);
            amplitude = 3.0f * oceanLambdaAndHeight[3] * sqrt(amplitude);
        }

        // cull breaking trochoids ( d(x+Acos(kx))=1-Akcos(); must be >0 )
        if (amplitude > 1.0f / knorm) amplitude = 1.0f / knorm;
        else if (amplitude < -1.0f / knorm) amplitude = -1.0f / knorm;

        waves[i].x() = amplitude; waves[i].y() = omega;
        waves[i].z() = knorm * cos(ktheta);
        waves[i].w() = knorm * sin(ktheta);
        sigmaXsq += pow(cos(ktheta), 2.0f) * (1.0f - sqrt(1.0f - knorm * knorm * amplitude * amplitude));
        sigmaYsq += pow(sin(ktheta), 2.0f) * (1.0f - sqrt(1.0f - knorm * knorm * amplitude * amplitude));
        oceanLambdaAndHeight[2] -= knorm * amplitude * amplitude * 0.5f;
        heightVariance += amplitude * amplitude * (2.0f - knorm * knorm * amplitude * amplitude) * 0.25f;
        amplitudeMax += fabs(amplitude);
    }

    float var = 4.0f;
    float h0 = oceanLambdaAndHeight[2] - var * sqrt(heightVariance);
    float h1 = oceanLambdaAndHeight[2] + var * sqrt(heightVariance);
    amplitudeMax = h1 - h0;

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(oceanWaveCount, 1, 1, GL_RGBA, GL_FLOAT);
    image->setInternalTextureFormat(GL_RGBA32F_ARB);
    memcpy(image->data(), waves.data(), waves.size() * sizeof(osg::Vec4));

    osg::ref_ptr<osg::Texture1D> tex = new osg::Texture1D;
    tex->setImage(image.get()); tex->setResizeNonPowerOfTwoHint(false);
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    if (oceanUniforms.find("SeaRoughness") != oceanUniforms.end())
        oceanUniforms["SeaRoughness"]->set(sigmaXsq);
    seaRoughness = sigmaXsq; return tex.release();
}

osg::Texture* EarthAtmosphereOcean::rawFloatingTexture2D(unsigned char* data, int w, int h, bool rgb)
{
    osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
    tex2D->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    tex2D->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    tex2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    tex2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    tex2D->setSourceType(GL_FLOAT);
    if (rgb) { tex2D->setInternalFormat(GL_RGB16F_ARB); tex2D->setSourceFormat(GL_RGB); }
    else { tex2D->setInternalFormat(GL_RGBA16F_ARB); tex2D->setSourceFormat(GL_RGBA); }

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->setImage(w, h, 1, tex2D->getInternalFormat(), tex2D->getSourceFormat(),
                    tex2D->getSourceType(), data, osg::Image::USE_NEW_DELETE);
    tex2D->setImage(image.get()); return tex2D.release();
}

osg::Texture* EarthAtmosphereOcean::rawFloatingTexture3D(unsigned char* data, int w, int h, int d, bool rgb)
{
    osg::ref_ptr<osg::Texture3D> tex3D = new osg::Texture3D;
    tex3D->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    tex3D->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    tex3D->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    tex3D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    tex3D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    tex3D->setSourceType(GL_FLOAT);
    if (rgb) { tex3D->setInternalFormat(GL_RGB16F_ARB); tex3D->setSourceFormat(GL_RGB); }
    else { tex3D->setInternalFormat(GL_RGBA16F_ARB); tex3D->setSourceFormat(GL_RGBA); }

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->setImage(w, h, d, tex3D->getInternalFormat(), tex3D->getSourceFormat(),
                    tex3D->getSourceType(), data, osg::Image::USE_NEW_DELETE);
    tex3D->setImage(image.get()); return tex3D.release();
}
