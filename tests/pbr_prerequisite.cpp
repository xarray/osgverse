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
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

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

int main(int argc, char** argv)
{
    std::string skyFile = SKYBOX_DIR + "barcelona.hdr";
    if (argc > 1) skyFile = argv[1];
    osg::Image* skyBox = osgDB::readImageFile(skyFile);
    int w = 1920, h = 1080;

    osg::ref_ptr<osg::Texture2D> hdrMap = osgVerse::createTexture2D(skyBox, osg::Texture::MIRROR);
    osg::Shader* vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "std_common_quad.vert.glsl");
    osg::Camera *cam0 = NULL, *cam2 = NULL;

    int cxtVer = 0, glslVer = 0; osgVerse::guessOpenGLVersions(cxtVer, glslVer);
    osgVerse::Pipeline::createShaderDefinitions(vs, cxtVer, glslVer);

    // BrdfLut
    osg::ref_ptr<osg::Image> img0 = new osg::Image;
    {
        osg::Shader* fs = osgDB::readShaderFile(
            osg::Shader::FRAGMENT, SHADER_DIR + "std_brdf_lut.frag.glsl");
        osgVerse::Pipeline::createShaderDefinitions(fs, cxtVer, glslVer);
        img0->allocateImage(w, h, 1, GL_RGB, GL_HALF_FLOAT);
        img0->setInternalTextureFormat(GL_RGB16F_ARB);

        osg::ref_ptr<osg::Program> prog = new osg::Program;
        prog->addShader(vs); prog->addShader(fs);
        cam0 = createRTTCameraForImage(osg::Camera::COLOR_BUFFER0, img0.get(), true);
        cam0->getOrCreateStateSet()->setAttributeAndModes(
            prog.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    }

    // Prefilter: one image per mip level, mip i <=> roughness i/4
    // (matching MAX_REFLECTION_LOD in std_pbr_lighting.frag.glsl)
    const int numPrefilterMips = 5;
    osg::ref_ptr<osg::Image> img1mips[5];
    osg::Camera* cam1mips[5] = { NULL };
    for (int i = 0; i < numPrefilterMips; ++i)
    {
        int mw = 512 >> i, mh = 256 >> i;
        osg::Shader* fs = osgDB::readShaderFile(
            osg::Shader::FRAGMENT, SHADER_DIR + "std_environment_prefiltering.frag.glsl");
        osgVerse::Pipeline::createShaderDefinitions(fs, cxtVer, glslVer);
        img1mips[i] = new osg::Image;
        img1mips[i]->allocateImage(mw, mh, 1, GL_RGB, GL_HALF_FLOAT);
        img1mips[i]->setInternalTextureFormat(GL_RGB16F_ARB);

        osg::ref_ptr<osg::Program> prog = new osg::Program;
        prog->addShader(vs); prog->addShader(fs);
        cam1mips[i] = createRTTCameraForImage(osg::Camera::COLOR_BUFFER0, img1mips[i].get(), true);
        cam1mips[i]->getOrCreateStateSet()->setAttributeAndModes(
            prog.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        cam1mips[i]->getOrCreateStateSet()->setTextureAttributeAndModes(0, hdrMap.get());
        cam1mips[i]->getOrCreateStateSet()->addUniform(new osg::Uniform("EnvironmentMap", (int)0));
        cam1mips[i]->getOrCreateStateSet()->addUniform(new osg::Uniform(
            "GlobalRoughness", (float)i / (float)(numPrefilterMips - 1)));
    }

    // IrrConvolution
    osg::ref_ptr<osg::Image> img2 = new osg::Image;
    {
        osg::Shader* fs = osgDB::readShaderFile(
            osg::Shader::FRAGMENT, SHADER_DIR + "std_irradiance_convolution.frag.glsl");
        osgVerse::Pipeline::createShaderDefinitions(fs, cxtVer, glslVer);
        img2->allocateImage(w, h, 1, GL_RGB, GL_UNSIGNED_BYTE);
        img2->setInternalTextureFormat(GL_RGB8);

        osg::ref_ptr<osg::Program> prog = new osg::Program;
        prog->addShader(vs); prog->addShader(fs);
        cam2 = createRTTCameraForImage(osg::Camera::COLOR_BUFFER0, img2.get(), true);
        cam2->getOrCreateStateSet()->setAttributeAndModes(
            prog.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        cam2->getOrCreateStateSet()->setTextureAttributeAndModes(0, hdrMap.get());
        cam2->getOrCreateStateSet()->addUniform(new osg::Uniform("EnvironmentMap", (int)0));
    }

    // Scene graph
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(cam0);
    for (int i = 0; i < numPrefilterMips; ++i) root->addChild(cam1mips[i]);
    root->addChild(cam2);

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    for (int i = 0; i < 3; ++i) viewer.frame();

    std::string outFile = osgDB::getNameLessExtension(skyFile);
#if 0
    osg::ref_ptr<osg::Texture2D> tex0 = osgVerse::createTexture2D(img0.get(), osg::Texture::MIRROR);
    osg::ref_ptr<osg::Texture2D> tex1 = osgVerse::createTexture2D(img1mips[0].get(), osg::Texture::MIRROR);
    osg::ref_ptr<osg::Texture2D> tex2 = osgVerse::createTexture2D(img2.get(), osg::Texture::MIRROR);

    osg::ref_ptr<osg::StateSet> savedSS = new osg::StateSet;
    savedSS->setTextureAttribute(0, tex0.get());
    savedSS->setTextureAttribute(1, tex1.get());
    savedSS->setTextureAttribute(2, tex2.get());
    osgDB::writeObjectFile(*savedSS, outFile + ".ibl.osgb");
    std::cout << "PBR textures output to " << outFile + ".ibl.osgb" << "\n";
#else
    // IBL data layout in the sequence: index 0 = BRDF LUT, index 1 = prefiltered environment map
    // of roughness 0, index 2 = irradiance convolution, and then the rest of the prefiltered
    // roughness levels (each one half the size of the previous). The renderer will pack them into a
    // single mipmap chain, so that different roughness values get different specular reflections.
    // https://github.com/xarray/osgverse/issues/7
    osg::ref_ptr<osg::ImageSequence> seq = new osg::ImageSequence;
    seq->addImage(img0); seq->addImage(img1mips[0]); seq->addImage(img2);
    for (int i = 1; i < numPrefilterMips; ++i) seq->addImage(img1mips[i]);
    if (osgDB::writeImageFile(*seq, outFile + ".ibl.rseq.verse_image"))
        std::cout << "PBR textures output to " << outFile + ".ibl.rseq" << "\n";
#endif
    return 0;
}
