#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture1D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <pipeline/Pipeline.h>
#include <pipeline/IncrementalCompiler.h>
#include <readerwriter/EarthManipulator.h>
#include <VerseCommon.h>
#include <iostream>
#include <sstream>

#define srnd() (2*frandom(&seed) - 1)
double radius = osg::WGS_84_RADIUS_EQUATOR, zmin = 20000.0;
int nbWaves = 60, resolution = 8;
float lambdaMin = 0.02f, lambdaMax = 30.0f;
float meanHeight = 0.0f, heightMax = 0.4f;//0.5;

extern std::map<std::string, osg::Uniform*> uniforms;
extern float oceanPixelScale;

#define SIMULATE_VERTEX 0
#if SIMULATE_VERTEX
namespace
{
#define NYQUIST_MIN 0.5
#define NYQUIST_MAX 1.25
    const float g = 9.81;

    float smoothstep(float edge0, float edge1, float x)
    {
        x = (x - edge0) / (edge1 - edge0);
        x = x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
        return x * x * (3.0f - 2.0f * x);
    }

    osg::Vec2 oceanPos(osg::StateSet* ss, const osg::Vec3& vertex, float& t, osg::Vec3& cameraDir, osg::Vec3& oceanDir)
    {
        osg::Matrixf screenToCamera; ss->getUniform("screenToCamera")->get(screenToCamera);
        osg::Matrixf cameraToOcean; ss->getUniform("cameraToOcean")->get(cameraToOcean);
        osg::Vec3 horizon1; ss->getUniform("horizon1")->get(horizon1);
        osg::Vec3 horizon2; ss->getUniform("horizon2")->get(horizon2);
        osg::Vec3 oceanCameraPos; ss->getUniform("oceanCameraPos")->get(oceanCameraPos);
        float heightOffset = 0.0f; ss->getUniform("heightOffset")->get(heightOffset);
        float radius = 0.0f; ss->getUniform("radius")->get(radius);

        float horizon = horizon1.x() + horizon1.y() * vertex.x() -
                        sqrt(horizon2.x() + (horizon2.y() + horizon2.z() * vertex.x()) * vertex.x());
        osg::Vec4 temp = osg::Vec4(vertex.x(), osg::minimum(vertex.y(), horizon), 0.0f, 1.0f) * screenToCamera;
        cameraDir = osg::Vec3(temp[0], temp[1], temp[2]); cameraDir.normalize();
        temp = osg::Vec4(cameraDir, 0.0) * cameraToOcean;
        oceanDir = osg::Vec3(temp[0], temp[1], temp[2]);

        float cz = oceanCameraPos.z(), dz = oceanDir.z();
        if (radius == 0.0)
            t = (heightOffset + 5.0 - cz) / dz;
        else
        {
            float b = dz * (cz + radius);
            float c = cz * (cz + 2.0 * radius);
            float tSphere = -b - sqrt(osg::maximum(b * b - c, 0.0f));
            float tApprox = -cz / dz * (1.0 + cz / (2.0 * radius) * (1.0 - dz * dz));
            t = abs((tApprox - tSphere) * dz) < 1.0 ? tApprox : tSphere;
        }
        return osg::Vec2(oceanCameraPos.x() + t * oceanDir.x(), oceanCameraPos.y() + t * oceanDir.y());
    }

    osg::Vec2 oceanPos(osg::StateSet* ss, const osg::Vec3& vertex)
    {
        float t = 0.0; osg::Vec3 cameraDir, oceanDir;
        return oceanPos(ss, vertex, t, cameraDir, oceanDir);
    }

    osg::Vec4 fakeVertexShader(osg::StateSet* ss, const osg::Vec3& osg_Vertex)
    {
        osg::Texture2D* tex = (osg::Texture2D*)ss->getTextureAttribute(5, osg::StateAttribute::TEXTURE);
        osg::Matrixf cameraToScreen; ss->getUniform("cameraToScreen")->get(cameraToScreen);
        osg::Matrixf oceanToCamera; ss->getUniform("oceanToCamera")->get(oceanToCamera);
        osg::Vec4 lods; ss->getUniform("lods")->get(lods);
        osg::Vec3 oceanCameraPos; ss->getUniform("oceanCameraPos")->get(oceanCameraPos);
        float heightOffset = 0.0f; ss->getUniform("heightOffset")->get(heightOffset);
        float seaRoughness = 0.0f; ss->getUniform("seaRoughness")->get(seaRoughness);
        float time = 0.0f; ss->getUniform("time")->get(time);

        float t = 0.0; osg::Vec3 cameraDir, oceanDir;
        osg::Vec2 uv = oceanPos(ss, osg_Vertex, t, cameraDir, oceanDir);

        float lod = -t / oceanDir.z() * lods.y();
        osg::Vec2 duv = oceanPos(ss, osg_Vertex + osg::Vec3(0.0, 0.01, 0.0)) - uv;
        osg::Vec3 dP = osg::Vec3(0.0, 0.0, heightOffset + (radius > 0.0 ? 0.0 : 5.0));
        osg::Vec3 dPdu = osg::Vec3(1.0, 0.0, 0.0), dPdv = osg::Vec3(0.0, 1.0, 0.0);
        float sigmaSq = seaRoughness;

        if (duv.x() != 0.0 || duv.y() != 0.0)
        {
            float iMin = osg::maximum(floor((log2(NYQUIST_MIN * lod) - lods.z()) * lods.w()), 0.0);
            for (float i = iMin; i < int(nbWaves); ++i)
            {
                osg::Vec4 wt = tex->getImage()->getColor(i, 0);
                float phase = wt.y() * time - osg::Vec2(wt.z(), wt.w()) * uv;
                float s = sin(phase), c = cos(phase);
                float overk = g / (wt.y() * wt.y());
                float wm = smoothstep(NYQUIST_MIN, NYQUIST_MAX, (2.0 * osg::PI) * overk / lod);
                osg::Vec3 factor = osg::Vec3(wt.z(), wt.w() * overk, 1.0) * wm * wt.x();
                dP += osg::Vec3(factor[0] * s, factor[1] * s, factor[2] * c);

                osg::Vec3 dPd = osg::Vec3(factor[0] * c, factor[1] * c, factor[2] * -s);
                dPdu -= dPd * wt.z(); dPdv -= dPd * wt.w();
                wt.z() *= overk; wt.w() *= overk;

                float kh = wt.x() / overk;
                sigmaSq -= wt.z() * wt.z() * (1.0 - sqrt(1.0 - kh * kh));
            }
        }

        osg::Vec3 p = oceanDir * t + dP + osg::Vec3(0.0, 0.0, oceanCameraPos.z());
        if (radius > 0.0)
        {
            dPdu += osg::Vec3(0.0, 0.0, -p.x() / (radius + p.z()));
            dPdv += osg::Vec3(0.0, 0.0, -p.y() / (radius + p.z()));
        }

        osg::Vec4 temp = osg::Vec4(dP, 1.0) * oceanToCamera;
        return osg::Vec4(cameraDir * t + osg::Vec3(temp[0], temp[1], temp[2]), 1.0) * cameraToScreen;
    }
}
#endif

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

osg::Texture* generateWaves(osg::StateSet* ss)
{
    long seed = 1234567;
    float min = log(lambdaMin) / log(2.0f);
    float max = log(lambdaMax) / log(2.0f);

    std::vector<osg::Vec4> waves(nbWaves);
    float sigmaXsq = 0.0f;
    float sigmaYsq = 0.0f;
    float heightVariance = 0.0f;
    float amplitudeMax = 0.0f;
    meanHeight = 0.0f;

#define nbAngles  5 // impair
#define angle(i)  (1.5 * (((i) % nbAngles) / (float)(nbAngles / 2) - 1))
#define dangle()   (1.5 / (float)(nbAngles / 2))
    float s = 0.0f;
    float Wa[nbAngles]; // normalised gaussian samples
    int index[nbAngles]; // to hash angle order
    for (int i = 0; i < nbAngles; i++)
    {
        index[i] = i; float a = angle(i); // (i/(float)(nbAngle/2)-1)*1.5;
        s += Wa[i] = exp(-.5 * a * a);
    }
    for (int i = 0; i < nbAngles; i++) Wa[i] /= s;

    const float waveDispersion = 0.9f;//6;
    const float U0 = 10.0f;
    const int spectrumType = 2;
    for (int i = 0; i < nbWaves; ++i)
    {
        float x = i / float(nbWaves - 1.0f);
        float lambda = pow(2.0f, (1.0f - x) * min + x * max);
        float ktheta = grandom(0.0f, 1.0f, &seed) * waveDispersion;
        float knorm = 2.0f * osg::PI / lambda;
        float omega = sqrt(9.81f * knorm);
        float amplitude = 0.0f;
        if (spectrumType == 1)
            amplitude = heightMax * grandom(0.5f, 0.15f, &seed) / (knorm * lambdaMax / (2.0f * osg::PI));
        else if (spectrumType == 2)
        {
            float step = (max - min) / (nbWaves - 1); // dlambda/di
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
            amplitude = (8.1e-3 * 9.81 * 9.81) / pow(omega, 5) * exp(-0.74 * pow(omega0 / omega, 4));
            amplitude *= .5 * sqrt(2 * 3.14 * 9.81 / lambda) * nbAngles * step; // (2/step-step/2);
            amplitude = 3 * heightMax * sqrt(amplitude);
        }

        // cull breaking trochoids ( d(x+Acos(kx))=1-Akcos(); must be >0 )
        if (amplitude > 1.0f / knorm) amplitude = 1.0f / knorm;
        else if (amplitude < -1.0f / knorm) amplitude = -1.0f / knorm;

        waves[i].x() = amplitude; waves[i].y() = omega;
        waves[i].z() = knorm * cos(ktheta);
        waves[i].w() = knorm * sin(ktheta);
        sigmaXsq += pow(cos(ktheta), 2.0f) * (1.0f - sqrt(1.0f - knorm * knorm * amplitude * amplitude));
        sigmaYsq += pow(sin(ktheta), 2.0f) * (1.0f - sqrt(1.0f - knorm * knorm * amplitude * amplitude));
        meanHeight -= knorm * amplitude * amplitude * 0.5f;
        heightVariance += amplitude * amplitude * (2.0f - knorm * knorm * amplitude * amplitude) * 0.25f;
        amplitudeMax += fabs(amplitude);
    }

    float var = 4.0f;
    float h0 = meanHeight - var * sqrt(heightVariance);
    float h1 = meanHeight + var * sqrt(heightVariance);
    amplitudeMax = h1 - h0;

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(nbWaves, 1, 1, GL_RGBA, GL_FLOAT);
    image->setInternalTextureFormat(GL_RGBA32F_ARB);
    memcpy(image->data(), waves.data(), waves.size() * sizeof(osg::Vec4));

    osg::ref_ptr<osg::Texture1D> tex = new osg::Texture1D;
    tex->setImage(image.get()); tex->setResizeNonPowerOfTwoHint(false);
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

    ss->addUniform(new osg::Uniform("seaColor", osg::Vec3(10.f / 255.f, 40.f / 255.f, 120.f / 255.f) * 0.1f));
    ss->addUniform(new osg::Uniform("seaRoughness", sigmaXsq));
    ss->addUniform(new osg::Uniform("nbWaves", (float)nbWaves));
    return tex.release();
}

osg::Geometry* createGrid(int w, int h)
{
    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);

    float f = 1.25f;
    int NX = int(f * w / resolution);
    int NY = int(f * h / resolution);
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
    geom->setVertexArray(va); geom->addPrimitiveSet(de);
    return geom;
}

class OceanCallback : public osg::NodeCallback
{
public:
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (!_view) return;
        osg::Camera* camera = _view->getCamera();
        osgVerse::EarthManipulator* manipulator =
            static_cast<osgVerse::EarthManipulator*>(_view->getCameraManipulator());

        osg::Matrix ctol = camera->getInverseViewMatrix();
        osg::Vec3d cl = osg::Vec3() * ctol;
        if ((radius == 0.0 && cl.z() > zmin) || (radius > 0.0 && cl.length() > radius + zmin))
        { oldLtoo = osg::Matrix(); offset = osg::Vec3(); }
        
        osg::Vec3d ux, uy, uz, oo;
        if (radius == 0.0)
        {
            ux = osg::X_AXIS; uy = osg::Y_AXIS; uz = osg::Z_AXIS;
            oo = osg::Vec3d(cl.x(), cl.y(), 0.0);
        }
        else
        {   // spherical ocean
            uz = cl; uz.normalize(); // unit z vector of ocean frame, in local space
            if (!oldLtoo.isIdentity())
            {
                ux = osg::Vec3d(oldLtoo(1, 0), oldLtoo(1, 1), oldLtoo(1, 2));
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
        osg::Matrix ctoo = ctol * ltoo;  // compute ctoo = cameraToOcean transform
        if (!oldLtoo.isIdentity())
        {
            osg::Vec3d delta = osg::Vec3d() * osg::Matrix::inverse(oldLtoo) * ltoo;
            offset += delta;
        }
        oldLtoo = ltoo;

        // get sun dir
        osg::Matrix ctos = camera->getProjectionMatrix();
        osg::Matrix stoc = osg::Matrix::inverse(ctos);
        osg::Matrix otoc = osg::Matrix::inverse(ctoo);
        otoc.setTrans(osg::Vec3());

        osg::Vec3 wDir, oDir;
        osg::StateSet* ss = node->getOrCreateStateSet();
        osg::Uniform* worldSunDir = ss->getOrCreateUniform("worldSunDir", osg::Uniform::FLOAT_VEC3);
        osg::Uniform* oceanSunDir = ss->getOrCreateUniform("oceanSunDir", osg::Uniform::FLOAT_VEC3);
        worldSunDir->get(wDir); oceanSunDir->set(osg::Matrixf::transform3x3(wDir, ltoo));

        osg::Vec3d oc = osg::Vec3d() * ctoo; float h = oc.z();  // if h < 0, we are under the ocean...
        manipulator->setZoomFactor(osg::Vec2(osg::clampBetween(h / 5000.0f, 0.1f, 1.0f), 1.0f));

        ss->getOrCreateUniform("underOcean", osg::Uniform::FLOAT)->set(h);
        ss->getOrCreateUniform("cameraToOcean", osg::Uniform::FLOAT_MAT4)->set(osg::Matrixf(ctoo));
        ss->getOrCreateUniform("screenToCamera", osg::Uniform::FLOAT_MAT4)->set(osg::Matrixf(stoc));
        ss->getOrCreateUniform("cameraToScreen", osg::Uniform::FLOAT_MAT4)->set(osg::Matrixf(ctos));
        ss->getOrCreateUniform("oceanToCamera", osg::Uniform::FLOAT_MAT4)->set(osg::Matrixf(otoc));
        ss->getOrCreateUniform("oceanToWorld", osg::Uniform::FLOAT_MAT4)->set(osg::Matrixf::inverse(ltoo));
        ss->getOrCreateUniform("oceanCameraPos", osg::Uniform::FLOAT_VEC3)
          ->set(osg::Vec3(-offset.x(), -offset.y(), h));

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
            ss->getOrCreateUniform("horizon1", osg::Uniform::FLOAT_VEC3)->set(
                osg::Vec3(-(h * 1e-6 + A0.z()) / B.z(), -dA.z() / B.z(), 0.0));
            ss->getOrCreateUniform("horizon2", osg::Uniform::FLOAT_VEC3)->set(osg::Vec3());
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
            ss->getOrCreateUniform("horizon1", osg::Uniform::FLOAT_VEC3)->set(osg::Vec3(-beta0, -beta1, alpha));
            ss->getOrCreateUniform("horizon2", osg::Uniform::FLOAT_VEC3)->set(
                osg::Vec3(beta0 * beta0 - gamma0, 2.0 * (beta0 * beta1 - gamma1), beta1 * beta1 - gamma2));
        }

        // angle under which a screen pixel is viewed from the camera
        float fov, aspectRatio, znear, zfar;
        float width = camera->getViewport() ? camera->getViewport()->width() : 1920;
        float height = camera->getViewport() ? camera->getViewport()->height() : 1080;
        camera->getProjectionMatrix().getPerspective(fov, aspectRatio, znear, zfar);

        // FIXME: pixelSize affects wave tiling, should be treated carefully
        float pixelSize = atan(tan(osg::inDegrees(fov * aspectRatio * oceanPixelScale)) / (height / 2.0f));
        ss->getOrCreateUniform("screenSize", osg::Uniform::FLOAT_VEC2)->set(osg::Vec2(width, height));
        ss->getOrCreateUniform("radius", osg::Uniform::FLOAT)->set((float)radius);
        ss->getOrCreateUniform("heightOffset", osg::Uniform::FLOAT)->set(-meanHeight);
        ss->getOrCreateUniform("lods", osg::Uniform::FLOAT_VEC4)->set(osg::Vec4(
            resolution, pixelSize * resolution, log(lambdaMin) / log(2.0f),
            (nbWaves - 1.0f) / (log(lambdaMax) / log(2.0f) - log(lambdaMin) / log(2.0f))));
        if (nv && nv->getFrameStamp())
        {
            float t = nv->getFrameStamp()->getSimulationTime();
            ss->getOrCreateUniform("time", osg::Uniform::FLOAT)->set((float)t * 3.0f);
        }

#if SIMULATE_VERTEX
        osg::Vec3Array* va = static_cast<osg::Vec3Array*>(_grid->getVertexArray());
        for (size_t i = 0; i < va->size(); ++i)
        {
            if ((*va)[i].x() > 0.2f && (*va)[i].x() < 0.8f &&
                (*va)[i].y() > 0.2f && (*va)[i].y() < 0.8f) {}
            else continue;

            osg::Vec4 v = fakeVertexShader(ss, (*va)[i]);
            std::cout << "VERTEX: " << (*va)[i] << ": " << v / v.w() << std::endl;
            break;
        }
#endif
        traverse(node, nv);
    }
    
    OceanCallback(osgViewer::View* view, osg::Geometry* grid)
        : _view(view), _grid(grid) {}

protected:
    osg::observer_ptr<osgViewer::View> _view;
    osg::observer_ptr<osg::Geometry> _grid;
    osg::Matrix oldLtoo;
    osg::Vec3d offset;
};

osg::Node* configureOcean(osgViewer::View& viewer, osg::Group* root, osg::Texture* sceneMaskTex,
                          const std::string& mainFolder, int width, int height, unsigned int mask)
{
    osg::Shader* vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "global_ocean.vert.glsl");
    osg::Shader* fs = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "global_ocean.frag.glsl");

    osg::ref_ptr<osg::Program> program = new osg::Program;
    vs->setName("Ocean_VS"); program->addShader(vs);
    fs->setName("Ocean_FS"); program->addShader(fs);
    osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);  // FIXME

    osg::Camera* hudCamera = osgVerse::createHUDCamera(NULL, width, height, osg::Vec3(), 1.0f, 1.0f, false);
    hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    hudCamera->setProjectionMatrix(osg::Matrix::ortho2D(-1.0, 1.0, -1.0, 1.0));
    hudCamera->setViewMatrix(osg::Matrix::identity());

    osg::StateSet* ss = root->getOrCreateStateSet();
    ss->setAttributeAndModes(program.get());
    ss->addUniform(new osg::Uniform("oceanOpaque", 1.0f));
    ss->addUniform(new osg::Uniform("wavesSampler", (int)7));
    ss->addUniform(new osg::Uniform("earthMaskSampler", (int)8));
    ss->setTextureAttributeAndModes(7, generateWaves(ss));
    ss->setTextureAttributeAndModes(8, sceneMaskTex);  // FIXME: no need to merge with root stateset and make big tex-units
                                                       // A shared stateset should be maintained for ocean, sky and ground..

    uniforms["seaColor"] = ss->getUniform("seaColor");

    osg::Geometry* grid = createGrid(width, height);
    osg::Geode* grideGeode = new osg::Geode;
    grideGeode->addDrawable(grid);
    hudCamera->addChild(grideGeode);
    hudCamera->setNodeMask(mask);
    hudCamera->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    hudCamera->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    root->addChild(hudCamera);
    root->addUpdateCallback(new OceanCallback(&viewer, grid));
    return hudCamera;
}
