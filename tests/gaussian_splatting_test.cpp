#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/BlendFunc>
#include <osg/BlendEquation>
#include <osg/Depth>
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
#include <pipeline/Utilities.h>
#include <VerseCommon.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

class GaussianStateVisitor : public osg::NodeVisitor
{
public:
    GaussianStateVisitor(osgVerse::GaussianSorter* s) : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN), _sorter(s)
    {
        osg::Shader* vert = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "gaussian_splatting.vert.glsl");
        osg::Shader* geom = osgDB::readShaderFile(osg::Shader::GEOMETRY, SHADER_DIR + "gaussian_splatting.geom.glsl");
        osg::Shader* frag = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "gaussian_splatting.frag.glsl");
        if (!vert || !geom || !frag)
        {
            OSG_WARN << "Missing shaders for gaussian splatting." << std::endl;
            return;
        }

        osgVerse::Pipeline::createShaderDefinitions(vert, 100, 430);
        osgVerse::Pipeline::createShaderDefinitions(geom, 100, 130);
        osgVerse::Pipeline::createShaderDefinitions(frag, 100, 130);  // FIXME
        _program = osgVerse::GaussianGeometry::createProgram(vert, geom, frag);
        _callback = osgVerse::GaussianGeometry::createUniformCallback();
    }

    virtual void apply(osg::Geode& node)
    {
        bool hasGaussian = false;
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
        {
            osgVerse::GaussianGeometry* gs = dynamic_cast<osgVerse::GaussianGeometry*>(node.getDrawable(i));
            if (gs && _sorter.valid())
            {   // to sort geometries by depth
                gs->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                _sorter->addGeometry(gs); hasGaussian = true;
            }
        }

        if (hasGaussian)
        {
            osg::StateSet* ss = node.getOrCreateStateSet();
            ss->setAttribute(_program.get());
            ss->setAttributeAndModes(new osg::BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
            //ss->setAttributeAndModes(new osg::BlendEquation(osg::BlendEquation::FUNC_ADD));
            ss->setAttributeAndModes(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false));
            ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
            node.setCullCallback(_callback.get());
        }
        traverse(node);
    }

protected:
    osg::ref_ptr<osg::Program> _program;
    osg::ref_ptr<osg::NodeCallback> _callback;
    osg::observer_ptr<osgVerse::GaussianSorter> _sorter;
};

class SortCallback : public osg::Camera::DrawCallback
{
public:
    SortCallback(osgVerse::GaussianSorter* sorter)
        : _sorter(sorter) {}

    virtual void operator()(osg::RenderInfo& renderInfo) const
    {
        //if (renderInfo.getView()->getFrameStamp()->getFrameNumber() % 10 != 0) return;
        if (renderInfo.getCurrentCamera() != NULL)
            _sorter->cull(renderInfo.getCurrentCamera()->getViewMatrix());
    }

protected:
    osg::ref_ptr<osgVerse::GaussianSorter> _sorter;
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgDB::Registry::instance()->addFileExtensionAlias("ply", "verse_3dgs");
    osgDB::Registry::instance()->addFileExtensionAlias("spz", "verse_3dgs");
    osgDB::Registry::instance()->addFileExtensionAlias("splat", "verse_3dgs");
    osgDB::Registry::instance()->addFileExtensionAlias("ksplat", "verse_3dgs");
    osgDB::Registry::instance()->addFileExtensionAlias("lcc", "verse_3dgs");
    osgDB::Registry::instance()->addFileExtensionAlias("sog", "verse_3dgs");
    osgVerse::updateOsgBinaryWrappers();

    osg::ref_ptr<osg::Node> gs = osgDB::readNodeFiles(arguments);
    if (!gs) gs = osgDB::readNodeFile(BASE_DIR + "/models/3dgs_parrot.splat");
    if (!gs) { std::cout << "No 3DGS file loaded" << std::endl; return 1; }

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->getOrCreateStateSet()->getOrCreateUniform("GaussianRenderingMode", osg::Uniform::FLOAT)->set(0.0f);
    root->addChild(gs.get());

    osgViewer::Viewer viewer;
    viewer.getCamera()->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    viewer.getCamera()->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());

    osgVerse::QuickEventHandler* handler = new osgVerse::QuickEventHandler;
    handler->addKeyUpCallback('1', [&](int key) { root->getStateSet()->getUniform("GaussianRenderingMode")->set(1.0f); });
    handler->addKeyUpCallback('0', [&](int key) { root->getStateSet()->getUniform("GaussianRenderingMode")->set(0.0f); });
    viewer.addEventHandler(handler);

    osg::ref_ptr<osgVerse::GaussianSorter> sorter = new osgVerse::GaussianSorter;  // TODO: better sort in GL context
    if (arguments.read("--gl46")) sorter->setMethod(osgVerse::GaussianSorter::GL46_RADIX_SORT);

    GaussianStateVisitor gsv(sorter.get()); gs->accept(gsv);
    viewer.getCamera()->setPreDrawCallback(new SortCallback(sorter.get()));

    int screenNo = 0; arguments.read("--screen", screenNo);
    viewer.setUpViewOnSingleScreen(screenNo);
    return viewer.run();
}
