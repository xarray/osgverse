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
        osg::ref_ptr<osgVerse::ParticleSystemU3D> ps = new osgVerse::ParticleSystemU3D;
        ps->setTexture(osgVerse::createTexture2D(
            osgDB::readImageFile(BASE_DIR + "/textures/foam_texture_sheet.png")));
        ps->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        ps->setGravityScale(0.0); ps->setMaxParticles(1200);
        ps->setDuration(3.0); ps->setAspectRatio(16.0 / 9.0);
        ps->setStartLifeRange(osg::Vec2(1.0f, 2.0f));
        ps->setStartSizeRange(osg::Vec2(2.0f, 5.0f));
        ps->setStartSpeedRange(osg::Vec2(0.0f, 0.0f));
        ps->setEmissionCount(osg::Vec2(100.0f, 0.0f));
        ps->setEmissionShape(osgVerse::ParticleSystemU3D::EMIT_Box);
        ps->setEmissionShapeCenter(osg::Vec3(0.0f, 6.0f, -2.5f));
        ps->setEmissionShapeValues(osg::Vec4(12.0f, 6.0f, 6.0f, 0.0f));
        ps->getColorPerTime()[0.0f] = osg::Vec4(0.4, 0.4, 0.4f, 0.0f);
        ps->getColorPerTime()[0.5f] = osg::Vec4(0.4, 0.4, 0.4f, 0.1f);
        ps->getColorPerTime()[1.0f] = osg::Vec4(0.4, 0.4, 0.4f, 0.0f);
        ps->setTextureSheetTiles(osg::Vec2(8.0f, 8.0f));
        ps->setTextureSheetValues(osg::Vec4(32.0f, 0.0f, 0.0f, 0.0f));
        ps->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        ps->linkTo(particleNode.get(), true,
                   osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "particles.vert.glsl"),
                   osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "particles.frag.glsl"));
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
