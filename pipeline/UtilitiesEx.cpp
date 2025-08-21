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

#include "ShaderLibrary.h"
#include "Pipeline.h"
#include "Utilities.h"
using namespace osgVerse;

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

void EarthAtmosphereOcean::create(const std::string& tr, const std::string& ir,
                                  const std::string& gl, const std::string& in)
{
    unsigned int trSize = 0, irSize = 0, glSize = 0, inSize = 0;
    unsigned char *trData = loadAllData(tr, trSize, 0), *irData = loadAllData(ir, irSize, 0);
    unsigned char *inData = loadAllData(in, inSize, 0);
    if (!trData || !irData || !inData) return;

    osg::ref_ptr<osg::Texture> trTex = rawFloatingTexture2D(trData, 256, 64, true);
    osg::ref_ptr<osg::Texture> irTex = rawFloatingTexture2D(irData, 64, 16, true);
    osg::ref_ptr<osg::Texture> inTex = rawFloatingTexture3D(inData, 256, 128, 32, false);
    osg::ref_ptr<osg::Texture> glTex = createTexture2D(osgDB::readImageFile(gl), osg::Texture::CLAMP);
    create(trTex.get(), irTex.get(), glTex.get(), inTex.get());
}

void EarthAtmosphereOcean::create(osg::Texture* tr, osg::Texture* ir,
                                  osg::Texture* gl, osg::Texture* in)
{
    transmittance = tr; irradiance = ir; glare = gl; inscatter = in;
    //commonUniforms[] = new osg::Uniform("TransmittanceSampler", (int)startU);
    //commonUniforms[] = new osg::Uniform("SkyIrradianceSampler", (int)startU + 1);
    //commonUniforms[] = new osg::Uniform("InscatterSampler", (int)startU + 2);
    //commonUniforms[] = new osg::Uniform("GlareSampler", (int)startU + 3);
    commonUniforms["EarthOrigin"] = new osg::Uniform("EarthOrigin", osg::Vec3(0.0f, 0.0f, 0.0f));
    commonUniforms["GlobalOpaque"] = new osg::Uniform("GlobalOpaque", 1.0f);
    commonUniforms["HdrExposure"] = new osg::Uniform("HdrExposure", 0.25f);
}

/** Helper functions to load raw floating texture data */
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
