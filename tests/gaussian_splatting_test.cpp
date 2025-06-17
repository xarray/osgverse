#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/BlendFunc>
#include <osg/DispatchCompute>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <modeling/Math.h>
#include <modeling/GaussianGeometry.h>
#include <pipeline/Pipeline.h>
#include <VerseCommon.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

class GaussianShaderVisitor : public osg::NodeVisitor
{
public:
    GaussianShaderVisitor() : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
    {
        osg::Shader* vert = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "gaussian_splatting.vert.glsl");
        osg::Shader* geom = osgDB::readShaderFile(osg::Shader::GEOMETRY, SHADER_DIR + "gaussian_splatting.geom.glsl");
        osg::Shader* frag = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "gaussian_splatting.frag.glsl");
        if (!vert || !geom || !frag)
        {
            OSG_WARN << "Missing shaders for gaussian splatting." << std::endl;
            return;
        }

        osgVerse::Pipeline::createShaderDefinitions(vert, 100, 130);
        osgVerse::Pipeline::createShaderDefinitions(geom, 100, 130);
        osgVerse::Pipeline::createShaderDefinitions(frag, 100, 130);
        _program = osgVerse::GaussianGeometry::createProgram(vert, geom, frag);
    }

    virtual void apply(osg::Geode& node)
    {
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
        {
            osgVerse::GaussianGeometry* gg = dynamic_cast<osgVerse::GaussianGeometry*>(node.getDrawable(i));
            if (gg) gg->getOrCreateStateSet()->setAttribute(_program.get());
        }
        traverse(node);
    }

protected:
    osg::ref_ptr<osg::Program> _program;
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();

    osg::ref_ptr<osg::Node> gs = osgDB::readNodeFile("../test.ply.verse_3dgs");
    if (!gs) return 1;

    GaussianShaderVisitor gsv; gs->accept(gsv);
    gs->setCullCallback(osgVerse::GaussianGeometry::createUniformCallback());
    gs->getOrCreateStateSet()->setAttributeAndModes(new osg::BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(gs.get());

    osgViewer::Viewer viewer;
    viewer.getCamera()->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setSceneData(root.get());
    return viewer.run();
}
