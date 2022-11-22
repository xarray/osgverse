#include <osg/io_utils>
#include <osg/LightSource>
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
    std::string skyFile = SKYBOX_DIR "barcelona.hdr";
    if (argc > 1) skyFile = argv[1];
    osg::Image* skyBox = osgDB::readImageFile(skyFile);
    int w = 1920, h = 1080;

    osg::ref_ptr<osg::Texture2D> hdrMap = osgVerse::createTexture2D(skyBox, osg::Texture::MIRROR);
    osg::Shader* vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "std_common_quad.vert.glsl");
    osg::Camera *cam0 = NULL, *cam1 = NULL, *cam2 = NULL;
    osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);

    // BrdfLut
    osg::ref_ptr<osg::Image> img0 = new osg::Image;
    {
        osg::Shader* fs = osgDB::readShaderFile(
            osg::Shader::FRAGMENT, SHADER_DIR "std_brdf_lut.frag.glsl");
        osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);
        img0->allocateImage(w, h, 1, GL_RGB, GL_HALF_FLOAT);
        img0->setInternalTextureFormat(GL_RGB16F_ARB);

        osg::ref_ptr<osg::Program> prog = new osg::Program;
        prog->addShader(vs); prog->addShader(fs);
        cam0 = createRTTCameraForImage(osg::Camera::COLOR_BUFFER, img0.get(), true);
        cam0->getOrCreateStateSet()->setAttributeAndModes(
            prog.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    }

    // Prefilter
    osg::ref_ptr<osg::Image> img1 = new osg::Image;
    {
        osg::Shader* fs = osgDB::readShaderFile(
            osg::Shader::FRAGMENT, SHADER_DIR "std_environment_prefiltering.frag.glsl");
        osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);
        img1->allocateImage(w, h, 1, GL_RGB, GL_UNSIGNED_BYTE);
        img1->setInternalTextureFormat(GL_RGB8);

        osg::ref_ptr<osg::Program> prog = new osg::Program;
        prog->addShader(vs); prog->addShader(fs);
        cam1 = createRTTCameraForImage(osg::Camera::COLOR_BUFFER, img1.get(), true);
        cam1->getOrCreateStateSet()->setAttributeAndModes(
            prog.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        cam1->getOrCreateStateSet()->setTextureAttributeAndModes(0, hdrMap.get());
        cam1->getOrCreateStateSet()->addUniform(new osg::Uniform("EnvironmentMap", (int)0));
        cam1->getOrCreateStateSet()->addUniform(new osg::Uniform("GlobalRoughness", 4.0f));
    }

    // IrrConvolution
    osg::ref_ptr<osg::Image> img2 = new osg::Image;
    {
        osg::Shader* fs = osgDB::readShaderFile(
            osg::Shader::FRAGMENT, SHADER_DIR "std_irradiance_convolution.frag.glsl");
        osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);
        img2->allocateImage(w, h, 1, GL_RGB, GL_UNSIGNED_BYTE);
        img2->setInternalTextureFormat(GL_RGB8);

        osg::ref_ptr<osg::Program> prog = new osg::Program;
        prog->addShader(vs); prog->addShader(fs);
        cam2 = createRTTCameraForImage(osg::Camera::COLOR_BUFFER, img2.get(), true);
        cam2->getOrCreateStateSet()->setAttributeAndModes(
            prog.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        cam2->getOrCreateStateSet()->setTextureAttributeAndModes(0, hdrMap.get());
        cam2->getOrCreateStateSet()->addUniform(new osg::Uniform("EnvironmentMap", (int)0));
    }

    // Scene graph
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(cam0);
    root->addChild(cam1);
    root->addChild(cam2);

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    for (int i = 0; i < 3; ++i) viewer.frame();

    /*osg::Vec3ub* ptr = (osg::Vec3ub*)img2->data();
    for (int y = 530; y < 550; ++y)
        for (int x = 950; x < 970; ++x)
        {
            osg::Vec3ub v = *(ptr + y * w + x);
            printf("%d, %d, %d\n", v[0], v[1], v[2]);
        }*/

    std::string outFile = osgDB::getNameLessExtension(skyFile);
    osg::ref_ptr<osg::Texture2D> tex0 = osgVerse::createTexture2D(img0.get(), osg::Texture::MIRROR);
    osg::ref_ptr<osg::Texture2D> tex1 = osgVerse::createTexture2D(img1.get(), osg::Texture::MIRROR);
    osg::ref_ptr<osg::Texture2D> tex2 = osgVerse::createTexture2D(img2.get(), osg::Texture::MIRROR);

    osg::ref_ptr<osg::StateSet> savedSS = new osg::StateSet;
    savedSS->setTextureAttribute(0, tex0.get());
    savedSS->setTextureAttribute(1, tex1.get());
    savedSS->setTextureAttribute(2, tex2.get());
    osgDB::writeObjectFile(*savedSS, outFile + ".ibl.osgb");
    std::cout << "PBR textures output to " << outFile + ".ibl.osgb" << "\n";
    return 0;
}
