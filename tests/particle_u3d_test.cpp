#include <osg/io_utils>
#include <osg/TriangleIndexFunctor>
#include <osg/Texture2D>
#include <osg/Depth>
#include <osg/MatrixTransform>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <animation/ParticleEngine.h>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
#endif

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Group> root = new osg::Group;

    osg::ref_ptr<osg::Node> scene =
        (argc < 2) ? osgDB::readNodeFile("cessna.osg") : osgDB::readNodeFiles(arguments);
    if (!scene) { OSG_WARN << "Failed to load " << (argc < 2) ? "" : argv[1]; return 1; }
    root->addChild(scene.get());

    osg::ref_ptr<osg::Geode> particleNode = new osg::Geode;
    {
        osg::ref_ptr<osg::Image> imgTurbulence = osgDB::readImageFile(BASE_DIR + "/textures/turbulence_smoke_sheet.png");
        osg::ref_ptr<osg::Image> imgFlames = osgDB::readImageFile(BASE_DIR + "/textures/flames_sheet.png");
        osg::ref_ptr<osg::Image> imgTakeoff = osgDB::readImageFile(BASE_DIR + "/textures/takeoff_smoke_sheet.png");

        osg::ref_ptr<osg::Shader> vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "particles.vert.glsl");
        osg::ref_ptr<osg::Shader> fs = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "particles.frag.glsl");

        osg::ref_ptr<osgVerse::ParticleSystemU3D> turbulenceSmoke = new osgVerse::ParticleSystemU3D;
        turbulenceSmoke->setTexture(osgVerse::createTexture2D(imgTurbulence.get()));
        turbulenceSmoke->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        turbulenceSmoke->setGravityScale(-1.0); turbulenceSmoke->setMaxParticles(2000);
        turbulenceSmoke->setDuration(3.0); turbulenceSmoke->setAspectRatio(16.0 / 9.0);
        turbulenceSmoke->setStartLifeRange(osg::Vec2(0.3f, 1.2f));
        turbulenceSmoke->setStartSizeRange(osg::Vec2(3.0f, 4.0f));
        turbulenceSmoke->setStartSpeedRange(osg::Vec2(8.0f, 8.0f));
        turbulenceSmoke->setEmissionCount(osg::Vec2(400.0f, 0.0f));
        turbulenceSmoke->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Sphere);
        turbulenceSmoke->setEmissionShapeCenter(osg::Vec3(0.0f, 0.0f, -0.2f));
        turbulenceSmoke->setEmissionShapeValues(osg::Vec4(5.0f, 5.0f, 1.0f, 0.0f));
        turbulenceSmoke->getColorPerTime()[0.0f] = osg::Vec4(1.0f, 0.4f, 0.4f, 0.0f);
        turbulenceSmoke->getColorPerTime()[0.3f] = osg::Vec4(1.0f, 0.4f, 0.4f, 0.1f);
        turbulenceSmoke->getColorPerTime()[1.0f] = osg::Vec4(1.0f, 0.4f, 0.4f, 0.0f);
        turbulenceSmoke->setTextureSheetTiles(osg::Vec2(8.0f, 8.0f));
        turbulenceSmoke->setTextureSheetValues(osg::Vec4(32.0f, 0.0f, 0.0f, 0.0f));
        turbulenceSmoke->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Modulate);
        turbulenceSmoke->linkTo(particleNode.get(), false, vs.get(), fs.get());

        osg::ref_ptr<osgVerse::ParticleSystemU3D> flamesA = new osgVerse::ParticleSystemU3D;
        flamesA->setTexture(osgVerse::createTexture2D(imgFlames.get()));
        flamesA->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        flamesA->setGravityScale(-0.01); flamesA->setMaxParticles(2000);
        flamesA->setDuration(1.0); flamesA->setAspectRatio(16.0 / 9.0);
        flamesA->setStartLifeRange(osg::Vec2(0.5f, 1.0f));
        flamesA->setStartSizeRange(osg::Vec2(0.8f, 1.5f));
        flamesA->setStartSpeedRange(osg::Vec2(0.5f, 1.2f));
        flamesA->setEmissionCount(osg::Vec2(100.0f, 0.0f));
        flamesA->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Sphere);
        flamesA->setEmissionShapeCenter(osg::Vec3(0.0f, 0.0f, -0.5f));
        flamesA->setEmissionShapeValues(osg::Vec4(0.6f, 0.6f, 0.1f, 0.0f));
        flamesA->getColorPerTime()[0.0f] = osg::Vec4(1.0f, 0.55f, 0.0f, 0.0f);
        flamesA->getColorPerTime()[0.3f] = osg::Vec4(1.0f, 0.55f, 0.0f, 0.1f);
        flamesA->getColorPerTime()[1.0f] = osg::Vec4(1.0f, 0.55f, 0.0f, 0.0f);
        flamesA->getScalePerTime()[0.0f] = 0.25f; flamesA->getScalePerTime()[1.0f] = 1.0f;
        flamesA->setTextureSheetTiles(osg::Vec2(6.0f, 5.0f));
        flamesA->setTextureSheetValues(osg::Vec4(15.0f, 0.0f, 0.0f, 0.0f));
        flamesA->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        flamesA->linkTo(particleNode.get(), false, vs.get(), fs.get());

        osg::ref_ptr<osgVerse::ParticleSystemU3D> flamesB = new osgVerse::ParticleSystemU3D;
        flamesB->setTexture(osgVerse::createTexture2D(imgFlames.get()));
        flamesB->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        flamesB->setGravityScale(-0.01); flamesB->setMaxParticles(2000);
        flamesB->setDuration(1.0); flamesB->setAspectRatio(16.0 / 9.0);
        flamesB->setStartLifeRange(osg::Vec2(0.5f, 1.0f));
        flamesB->setStartSizeRange(osg::Vec2(1.0f, 3.0f));
        flamesB->setStartSpeedRange(osg::Vec2(1.2f, 6.0f));
        flamesB->setEmissionCount(osg::Vec2(600.0f, 0.0f));
        flamesB->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Sphere);
        flamesB->setEmissionShapeCenter(osg::Vec3(0.0f, 0.0f, -0.5f));
        flamesB->setEmissionShapeValues(osg::Vec4(0.8f, 0.8f, 0.1f, 0.0f));
        flamesB->getColorPerTime()[0.0f] = osg::Vec4(0.8f, 0.35f, 0.0f, 0.0f);
        flamesB->getColorPerTime()[0.3f] = osg::Vec4(0.8f, 0.35f, 0.0f, 0.1f);
        flamesB->getColorPerTime()[1.0f] = osg::Vec4(0.8f, 0.35f, 0.0f, 0.0f);
        flamesB->getScalePerTime()[0.0f] = 0.25f; flamesB->getScalePerTime()[1.0f] = 1.0f;
        flamesB->setTextureSheetTiles(osg::Vec2(6.0f, 5.0f));
        flamesB->setTextureSheetValues(osg::Vec4(15.0f, 0.0f, 0.0f, 0.0f));
        flamesB->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        flamesB->linkTo(particleNode.get(), false, vs.get(), fs.get());

        osg::ref_ptr<osgVerse::ParticleSystemU3D> takeOffSmoke = new osgVerse::ParticleSystemU3D;
        takeOffSmoke->setTexture(osgVerse::createTexture2D(imgTakeoff.get()));
        takeOffSmoke->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        takeOffSmoke->setGravityScale(-0.01); takeOffSmoke->setMaxParticles(2000);
        takeOffSmoke->setDuration(6.0); takeOffSmoke->setAspectRatio(16.0 / 9.0);
        takeOffSmoke->setStartLifeRange(osg::Vec2(4.0f, 6.0f));
        takeOffSmoke->setStartSizeRange(osg::Vec2(2.0f, 4.0f));
        takeOffSmoke->setStartSpeedRange(osg::Vec2(0.25f, 0.9f));
        takeOffSmoke->setEmissionCount(osg::Vec2(40.0f, 0.0f));
        takeOffSmoke->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Sphere);
        takeOffSmoke->setEmissionShapeCenter(osg::Vec3(0.0f, 0.0f, -0.5f));
        takeOffSmoke->setEmissionShapeValues(osg::Vec4(1.2f, 1.2f, 0.1f, 0.0f));
        takeOffSmoke->getColorPerTime()[0.0f] = osg::Vec4(0.8f, 0.35f, 0.0f, 0.0f);
        takeOffSmoke->getColorPerTime()[0.3f] = osg::Vec4(0.8f, 0.35f, 0.0f, 0.1f);
        takeOffSmoke->getColorPerTime()[1.0f] = osg::Vec4(0.8f, 0.35f, 0.0f, 0.0f);
        takeOffSmoke->getScalePerTime()[0.0f] = 0.5f; takeOffSmoke->getScalePerTime()[1.0f] = 1.0f;
        takeOffSmoke->setTextureSheetTiles(osg::Vec2(8.0f, 8.0f));
        takeOffSmoke->setTextureSheetValues(osg::Vec4(32.0f, 0.0f, 0.0f, 0.0f));
        takeOffSmoke->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        takeOffSmoke->linkTo(particleNode.get(), true, vs.get(), fs.get());
    }

    osg::ref_ptr<osg::MatrixTransform> particleMT = new osg::MatrixTransform;
    particleMT->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::Z_AXIS) *
                          osg::Matrix::translate(0.0f, 0.0f, 10.0f));
    particleMT->addChild(particleNode.get());
    root->addChild(particleMT.get());

    // Start the main loop
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
