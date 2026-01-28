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

#include <animation/TweenAnimation.h>
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
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osg::ref_ptr<osg::Group> root = new osg::Group;
    bool useGeomShader = arguments.read("--with-geometry-shader");

    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments);
    if (!scene) scene = osgDB::readNodeFile("cessna.osgt.0,0,-10.trans");
    if (!scene) { OSG_WARN << "Failed to load scene model " << (argc < 2) ? "" : argv[1]; }

    std::map<std::string, osgVerse::ParticleSystemU3D*> particleSystems;
    osg::ref_ptr<osg::Geode> particleNodeA = new osg::Geode;
    osg::ref_ptr<osg::Geode> particleNodeB = new osg::Geode;
    {
        osg::ref_ptr<osg::Image> imgTurbulence = osgDB::readImageFile(BASE_DIR + "/textures/turbulence_smoke_sheet.png");
        osg::ref_ptr<osg::Image> imgFlames = osgDB::readImageFile(BASE_DIR + "/textures/flames_sheet.png");
        osg::ref_ptr<osg::Image> imgTakeoff = osgDB::readImageFile(BASE_DIR + "/textures/takeoff_smoke_sheet.png");

        osg::ref_ptr<osg::Shader> vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "particles.vert.glsl");
        osg::ref_ptr<osg::Shader> fs = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "particles.frag.glsl");
        osg::ref_ptr<osg::Shader> gs = osgDB::readShaderFile(osg::Shader::GEOMETRY, SHADER_DIR + "particles.geom.glsl");
        osgVerse::ParticleSystemU3D::UpdateMethod method = useGeomShader ?
            osgVerse::ParticleSystemU3D::GPU_GEOMETRY : osgVerse::ParticleSystemU3D::CPU_VERTEX_ATTRIB;

        osg::ref_ptr<osgVerse::ParticleSystemU3D> turbulenceSmoke = new osgVerse::ParticleSystemU3D(method);
        turbulenceSmoke->setTexture(osgVerse::createTexture2D(imgTurbulence.get()));
        turbulenceSmoke->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        turbulenceSmoke->setGravityScale(-1.0); turbulenceSmoke->setMaxParticles(2000);
        turbulenceSmoke->setDuration(3.0); turbulenceSmoke->setAspectRatio(16.0 / 9.0);
        turbulenceSmoke->setStartLifeRange(osg::Vec2(0.3f, 1.2f));
        turbulenceSmoke->setStartSizeRange(osg::Vec2(3.0f, 5.0f));
        turbulenceSmoke->setStartSpeedRange(osg::Vec2(1.0f, 1.2f));
        turbulenceSmoke->setEmissionCount(osg::Vec2(400.0f, 0.0f));
        turbulenceSmoke->setEmissionSurface(osgVerse::ParticleSystemU3D::EMIT_Shell);
        turbulenceSmoke->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Circle);
        turbulenceSmoke->setEmissionShapeCenter(osg::Vec3(0.0f, 0.0f, -0.2f));
        turbulenceSmoke->setEmissionShapeValues(osg::Vec4(5.0f, 5.0f, 1.0f, 0.0f));
        turbulenceSmoke->getColorPerTime()[0.0f] = osg::Vec4(1.0f, 0.4f, 0.4f, 0.0f);
        turbulenceSmoke->getColorPerTime()[0.3f] = osg::Vec4(1.0f, 0.4f, 0.4f, 1.0f);
        turbulenceSmoke->getColorPerTime()[1.0f] = osg::Vec4(1.0f, 0.4f, 0.4f, 0.0f);
        turbulenceSmoke->setTextureSheetTiles(osg::Vec2(8.0f, 8.0f));
        turbulenceSmoke->setTextureSheetValues(osg::Vec4(32.0f, 0.0f, 0.0f, 0.0f));
        turbulenceSmoke->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Modulate);
        turbulenceSmoke->linkTo(particleNodeA.get(), false, vs.get(), fs.get(), gs.get());
        particleSystems["turbulenceSmoke"] = turbulenceSmoke.get();

        osg::ref_ptr<osgVerse::ParticleSystemU3D> flamesA = new osgVerse::ParticleSystemU3D(method);
        flamesA->setTexture(osgVerse::createTexture2D(imgFlames.get()));
        flamesA->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        flamesA->setGravityScale(-0.01); flamesA->setMaxParticles(2000);
        flamesA->setDuration(1.0); flamesA->setAspectRatio(16.0 / 9.0);
        flamesA->setStartLifeRange(osg::Vec2(0.5f, 1.0f));
        flamesA->setStartSizeRange(osg::Vec2(2.8f, 4.5f));
        flamesA->setStartSpeedRange(osg::Vec2(0.05f, 0.1f));
        flamesA->setEmissionCount(osg::Vec2(100.0f, 0.0f));
        flamesA->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Circle);
        flamesA->setEmissionShapeCenter(osg::Vec3(0.0f, 0.0f, -0.5f));
        flamesA->setEmissionShapeValues(osg::Vec4(0.6f, 0.6f, 0.1f, 0.0f));
        flamesA->getColorPerTime()[0.0f] = osg::Vec4(1.0f, 0.55f, 0.0f, 0.0f);
        flamesA->getColorPerTime()[0.3f] = osg::Vec4(1.0f, 0.55f, 0.0f, 1.0f);
        flamesA->getColorPerTime()[1.0f] = osg::Vec4(1.0f, 0.55f, 0.0f, 0.0f);
        flamesA->getScalePerTime()[0.0f] = 0.25f; flamesA->getScalePerTime()[1.0f] = 1.0f;
        flamesA->setTextureSheetTiles(osg::Vec2(6.0f, 5.0f));
        flamesA->setTextureSheetValues(osg::Vec4(15.0f, 0.0f, 0.0f, 0.0f));
        flamesA->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        flamesA->linkTo(particleNodeA.get(), false, vs.get(), fs.get(), gs.get());
        particleSystems["flamesA"] = flamesA.get();

        osg::ref_ptr<osgVerse::ParticleSystemU3D> flamesB = new osgVerse::ParticleSystemU3D(method);
        flamesB->setTexture(osgVerse::createTexture2D(imgFlames.get()));
        flamesB->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        flamesB->setGravityScale(-0.01); flamesB->setMaxParticles(2000);
        flamesB->setDuration(1.0); flamesB->setAspectRatio(16.0 / 9.0);
        flamesB->setStartLifeRange(osg::Vec2(0.5f, 1.0f));
        flamesB->setStartSizeRange(osg::Vec2(4.0f, 6.0f));
        flamesB->setStartSpeedRange(osg::Vec2(0.1f, 0.25f));
        flamesB->setEmissionCount(osg::Vec2(600.0f, 0.0f));
        flamesB->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Circle);
        flamesB->setEmissionShapeCenter(osg::Vec3(0.0f, 0.0f, -0.5f));
        flamesB->setEmissionShapeValues(osg::Vec4(0.8f, 0.8f, 0.1f, 0.0f));
        flamesB->getColorPerTime()[0.0f] = osg::Vec4(0.8f, 0.35f, 0.0f, 0.0f);
        flamesB->getColorPerTime()[0.3f] = osg::Vec4(0.8f, 0.35f, 0.0f, 1.0f);
        flamesB->getColorPerTime()[1.0f] = osg::Vec4(0.8f, 0.35f, 0.0f, 0.0f);
        flamesB->getScalePerTime()[0.0f] = 0.25f; flamesB->getScalePerTime()[1.0f] = 1.0f;
        flamesB->setTextureSheetTiles(osg::Vec2(6.0f, 5.0f));
        flamesB->setTextureSheetValues(osg::Vec4(15.0f, 0.0f, 0.0f, 0.0f));
        flamesB->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        flamesB->linkTo(particleNodeA.get(), true, vs.get(), fs.get(), gs.get());
        particleSystems["flamesB"] = flamesB.get();

        osg::ref_ptr<osgVerse::ParticleSystemU3D> takeOffSmoke = new osgVerse::ParticleSystemU3D(method);
        takeOffSmoke->setTexture(osgVerse::createTexture2D(imgTakeoff.get()));
        takeOffSmoke->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        takeOffSmoke->setGravityScale(0.1); takeOffSmoke->setMaxParticles(2000);
        takeOffSmoke->setDuration(6.0); takeOffSmoke->setAspectRatio(16.0 / 9.0);
        takeOffSmoke->setStartLifeRange(osg::Vec2(4.0f, 6.0f));
        takeOffSmoke->setStartSizeRange(osg::Vec2(132.0f, 248.0f));
        takeOffSmoke->setStartSpeedRange(osg::Vec2(0.01f, 0.02f));
        takeOffSmoke->setStartAttitude0(osg::Quat(-osg::PI_4, osg::X_AXIS));
        takeOffSmoke->setStartAttitude1(osg::Quat(-osg::PI_2, osg::X_AXIS));
        takeOffSmoke->setEmissionCount(osg::Vec2(40.0f, 0.0f));
        takeOffSmoke->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Circle);
        takeOffSmoke->setEmissionShapeCenter(osg::Vec3(0.0f, 0.0f, -0.5f));
        takeOffSmoke->setEmissionShapeValues(osg::Vec4(1.0f, 1.0f, 0.1f, 0.0f));
        takeOffSmoke->getColorPerTime()[0.0f] = osg::Vec4(0.8f, 0.15f, 0.0f, 0.0f);
        takeOffSmoke->getColorPerTime()[0.3f] = osg::Vec4(1.0f, 1.0f, 1.0f, 0.4f);
        takeOffSmoke->getColorPerTime()[1.0f] = osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        takeOffSmoke->getScalePerTime()[0.0f] = 0.3f; takeOffSmoke->getScalePerTime()[1.0f] = 1.0f;
        takeOffSmoke->setTextureSheetTiles(osg::Vec2(8.0f, 8.0f));
        takeOffSmoke->setTextureSheetValues(osg::Vec4(32.0f, 0.0f, 0.0f, 0.0f));
        takeOffSmoke->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Modulate);
        takeOffSmoke->linkTo(particleNodeB.get(), true, vs.get(), fs.get(), gs.get());
        particleSystems["takeOffSmoke"] = takeOffSmoke.get();
    }

    osg::ref_ptr<osg::MatrixTransform> particleMT = new osg::MatrixTransform;
    //particleMT->setMatrix(osg::Matrix::translate(20.0f, 0.0f, 0.0f));
    particleMT->addChild(particleNodeA.get());
    particleMT->addChild(scene.get());
    root->addChild(particleMT.get());
    root->addChild(particleNodeB.get());

    osgVerse::QuickEventHandler* handler = new osgVerse::QuickEventHandler;
    particleSystems["takeOffSmoke"]->stop();
    handler->addKeyUpCallback('x', [&](int key) {
        particleSystems["takeOffSmoke"]->play();
        particleSystems["turbulenceSmoke"]->stop();
        particleSystems["flamesA"]->stop();
        particleSystems["flamesB"]->stop();
    });

    // Start the main loop
    osgViewer::Viewer viewer;
    viewer.addEventHandler(handler);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
