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

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

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
        osg::ref_ptr<osg::Image> image = osgDB::readImageFile(BASE_DIR + "/textures/foam_texture_sheet.dds");
        osg::ref_ptr<osg::Shader> vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "particles.vert.glsl");
        osg::ref_ptr<osg::Shader> fs = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "particles.frag.glsl");

        osg::ref_ptr<osgVerse::ParticleSystemU3D> ps0 = new osgVerse::ParticleSystemU3D;
        ps0->setTexture(osgVerse::createTexture2D(image.get()));
        ps0->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        ps0->setGravityScale(0.0); ps0->setMaxParticles(1200);
        ps0->setDuration(3.0); ps0->setAspectRatio(16.0 / 9.0);
        ps0->setStartLifeRange(osg::Vec2(1.0f, 2.0f));
        ps0->setStartSizeRange(osg::Vec2(2.0f, 5.0f));
        ps0->setStartSpeedRange(osg::Vec2(0.0f, 0.0f));
        ps0->setEmissionCount(osg::Vec2(100.0f, 0.0f));
        ps0->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Box);
        ps0->setEmissionShapeCenter(osg::Vec3(0.0f, 6.0f, -2.5f));
        ps0->setEmissionShapeValues(osg::Vec4(12.0f, 6.0f, 6.0f, 0.0f));
        ps0->getColorPerTime()[0.0f] = osg::Vec4(1.0f, 0.4f, 0.4f, 0.0f);
        ps0->getColorPerTime()[0.5f] = osg::Vec4(1.0f, 0.4f, 0.4f, 0.1f);
        ps0->getColorPerTime()[1.0f] = osg::Vec4(1.0f, 0.4f, 0.4f, 0.0f);
        ps0->setTextureSheetTiles(osg::Vec2(8.0f, 8.0f));
        ps0->setTextureSheetValues(osg::Vec4(32.0f, 0.0f, 0.0f, 0.0f));
        ps0->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        ps0->linkTo(particleNode.get(), false, vs.get(), fs.get());

        osg::ref_ptr<osgVerse::ParticleSystemU3D> ps1 = new osgVerse::ParticleSystemU3D;
        ps1->setTexture(osgVerse::createTexture2D(image.get()));
        ps1->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        ps1->setGravityScale(0.0); ps1->setMaxParticles(1200);
        ps1->setDuration(3.0); ps1->setAspectRatio(16.0 / 9.0);
        ps1->setStartLifeRange(osg::Vec2(1.0f, 2.0f));
        ps1->setStartSizeRange(osg::Vec2(2.0f, 5.0f));
        ps1->setStartSpeedRange(osg::Vec2(0.0f, 0.0f));
        ps1->setEmissionCount(osg::Vec2(100.0f, 0.0f));
        ps1->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Box);
        ps1->setEmissionShapeCenter(osg::Vec3(0.0f, -6.0f, -2.5f));
        ps1->setEmissionShapeValues(osg::Vec4(12.0f, 6.0f, 6.0f, 0.0f));
        ps1->getColorPerTime()[0.0f] = osg::Vec4(0.4f, 1.0f, 0.4f, 0.0f);
        ps1->getColorPerTime()[0.5f] = osg::Vec4(0.4f, 1.0f, 0.4f, 0.1f);
        ps1->getColorPerTime()[1.0f] = osg::Vec4(0.4f, 1.0f, 0.4f, 0.0f);
        ps1->setTextureSheetTiles(osg::Vec2(8.0f, 8.0f));
        ps1->setTextureSheetValues(osg::Vec4(32.0f, 0.0f, 0.0f, 0.0f));
        ps1->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        ps1->linkTo(particleNode.get(), false, vs.get(), fs.get());

        osg::ref_ptr<osgVerse::ParticleSystemU3D> ps2 = new osgVerse::ParticleSystemU3D;
        ps2->setTexture(osgVerse::createTexture2D(image.get()));
        ps2->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        ps2->setGravityScale(0.0); ps2->setMaxParticles(1200);
        ps2->setDuration(3.0); ps2->setAspectRatio(16.0 / 9.0);
        ps2->setStartLifeRange(osg::Vec2(1.0f, 2.0f));
        ps2->setStartSizeRange(osg::Vec2(2.0f, 5.0f));
        ps2->setStartSpeedRange(osg::Vec2(0.0f, 0.0f));
        ps2->setEmissionCount(osg::Vec2(100.0f, 0.0f));
        ps2->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Box);
        ps2->setEmissionShapeCenter(osg::Vec3(6.0f, 0.0f, -2.5f));
        ps2->setEmissionShapeValues(osg::Vec4(12.0f, 6.0f, 6.0f, 0.0f));
        ps2->getColorPerTime()[0.0f] = osg::Vec4(0.4f, 0.4f, 1.0f, 0.0f);
        ps2->getColorPerTime()[0.5f] = osg::Vec4(0.4f, 0.4f, 1.0f, 0.1f);
        ps2->getColorPerTime()[1.0f] = osg::Vec4(0.4f, 0.4f, 1.0f, 0.0f);
        ps2->setTextureSheetTiles(osg::Vec2(8.0f, 8.0f));
        ps2->setTextureSheetValues(osg::Vec4(32.0f, 0.0f, 0.0f, 0.0f));
        ps2->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        ps2->linkTo(particleNode.get(), false, vs.get(), fs.get());

        osg::ref_ptr<osgVerse::ParticleSystemU3D> ps3 = new osgVerse::ParticleSystemU3D;
        ps3->setTexture(osgVerse::createTexture2D(image.get()));
        ps3->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        ps3->setGravityScale(0.0); ps3->setMaxParticles(1200);
        ps3->setDuration(3.0); ps3->setAspectRatio(16.0 / 9.0);
        ps3->setStartLifeRange(osg::Vec2(1.0f, 2.0f));
        ps3->setStartSizeRange(osg::Vec2(2.0f, 5.0f));
        ps3->setStartSpeedRange(osg::Vec2(0.0f, 0.0f));
        ps3->setEmissionCount(osg::Vec2(100.0f, 0.0f));
        ps3->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Box);
        ps3->setEmissionShapeCenter(osg::Vec3(-6.0f, 0.0f, -2.5f));
        ps3->setEmissionShapeValues(osg::Vec4(12.0f, 6.0f, 6.0f, 0.0f));
        ps3->getColorPerTime()[0.0f] = osg::Vec4(0.4f, 0.4f, 0.4f, 0.0f);
        ps3->getColorPerTime()[0.5f] = osg::Vec4(0.4f, 0.4f, 0.4f, 0.1f);
        ps3->getColorPerTime()[1.0f] = osg::Vec4(0.4f, 0.4f, 0.4f, 0.0f);
        ps3->setTextureSheetTiles(osg::Vec2(8.0f, 8.0f));
        ps3->setTextureSheetValues(osg::Vec4(32.0f, 0.0f, 0.0f, 0.0f));
        ps3->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        ps3->linkTo(particleNode.get(), true, vs.get(), fs.get());
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
