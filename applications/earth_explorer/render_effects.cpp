#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <readerwriter/EarthManipulator.h>
#include <readerwriter/DatabasePager.h>
#include <pipeline/Pipeline.h>
#include <VerseCommon.h>
#include <iostream>
#include <sstream>

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

static osg::Texture* createRawTexture2D(unsigned char* data, int w, int h, bool rgb)
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
    tex2D->setImage(image.get());
    return tex2D.release();
}

static osg::Texture* createRawTexture3D(unsigned char* data, int w, int h, int d, bool rgb)
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
    tex3D->setImage(image.get());
    return tex3D.release();
}

osg::Camera* configureEarthAndAtmosphere(osg::Group* root, osg::Node* earth, int width, int height)
{
    // Create RTT camera to render the globe
    osg::Shader* vs1 = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "scattering_globe.vert.glsl");
    osg::Shader* fs1 = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "scattering_globe.frag.glsl");

    osg::ref_ptr<osg::Program> program1 = new osg::Program;
    vs1->setName("Scattering_Globe_VS"); program1->addShader(vs1);
    fs1->setName("Scattering_Globe_FS"); program1->addShader(fs1);
    osgVerse::Pipeline::createShaderDefinitions(vs1, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs1, 100, 130);  // FIXME

    osg::ref_ptr<osg::Texture> rttBuffer =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, width, height);
    rttBuffer->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    rttBuffer->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    rttBuffer->setWrap(osg::Texture2D::WRAP_S, osg::Texture::CLAMP);
    rttBuffer->setWrap(osg::Texture2D::WRAP_T, osg::Texture::CLAMP);

    osg::Camera* rttCamera = osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER0, NULL, NULL, false);
    rttCamera->setViewport(0, 0, rttBuffer->getTextureWidth(), rttBuffer->getTextureHeight());
    rttCamera->attach(osg::Camera::COLOR_BUFFER0, rttBuffer.get(), 0, 0, false, 16, 4);
    earth->getOrCreateStateSet()->setAttributeAndModes(program1.get());

    // Create the atmosphere HUD
    osg::Shader* vs2 = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "scattering_sky.vert.glsl");
    osg::Shader* fs2 = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "scattering_sky.frag.glsl");

    osg::ref_ptr<osg::Program> program2 = new osg::Program;
    vs2->setName("Scattering_Sky_VS"); program2->addShader(vs2);
    fs2->setName("Scattering_Sky_FS"); program2->addShader(fs2);
    osgVerse::Pipeline::createShaderDefinitions(vs2, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs2, 100, 130);  // FIXME

    osg::Camera* hudCamera = osgVerse::createHUDCamera(NULL, width, height, osg::Vec3(), 1.0f, 1.0f, true);
    hudCamera->getOrCreateStateSet()->setAttributeAndModes(program2.get());
    hudCamera->getOrCreateStateSet()->setTextureAttributeAndModes(0, rttBuffer.get());

    // Setup global stateset
    unsigned int size = 0;
    unsigned char* transmittance = loadAllData(BASE_DIR + "/textures/transmittance.raw", size, 0);
    unsigned char* irradiance = loadAllData(BASE_DIR + "/textures/irradiance.raw", size, 0);
    unsigned char* inscatter = loadAllData(BASE_DIR + "/textures/inscatter.raw", size, 0);

    osg::StateSet* ss = root->getOrCreateStateSet();
    ss->setTextureAttributeAndModes(0, osgVerse::createDefaultTexture());
    ss->setTextureAttributeAndModes(1, osgVerse::createTexture2D(
        osgDB::readImageFile(BASE_DIR + "/textures/sunglare.png"), osg::Texture::CLAMP));
    ss->setTextureAttributeAndModes(2, createRawTexture2D(transmittance, 256, 64, true));
    ss->setTextureAttributeAndModes(3, createRawTexture2D(irradiance, 64, 16, true));
    ss->setTextureAttributeAndModes(4, createRawTexture3D(inscatter, 256, 128, 32, false));
    ss->addUniform(new osg::Uniform("sceneSampler", (int)0));
    ss->addUniform(new osg::Uniform("glareSampler", (int)1));
    ss->addUniform(new osg::Uniform("transmittanceSampler", (int)2));
    ss->addUniform(new osg::Uniform("skyIrradianceSampler", (int)3));
    ss->addUniform(new osg::Uniform("inscatterSampler", (int)4));
    ss->addUniform(new osg::Uniform("origin", osg::Vec3(0.0f, 0.0f, 0.0f)));
    ss->addUniform(new osg::Uniform("hdrExposure", 0.25f));
    ss->addUniform(new osg::Uniform("opaque", 1.0f));

    // Finish configuration
    rttCamera->addChild(earth);
    root->addChild(rttCamera);
    root->addChild(hudCamera);
    return rttCamera;
}
