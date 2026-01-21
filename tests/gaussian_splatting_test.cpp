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
#include <pipeline/ResourceManager.h>
#include <pipeline/Utilities.h>
#include <VerseCommon.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

class GaussianStateVisitor : public osg::NodeVisitor
{
public:
    GaussianStateVisitor(osgVerse::GaussianSorter* s, const std::string& hint)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN), _sorter(s)
    {
        osg::Shader* vert = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "gaussian_splatting.vert.glsl");
        osg::Shader* geom = osgDB::readShaderFile(osg::Shader::GEOMETRY, SHADER_DIR + "gaussian_splatting.geom.glsl");
        osg::Shader* frag = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "gaussian_splatting.frag.glsl");
        if (!vert || !geom || !frag)
        {
            OSG_WARN << "Missing shaders for gaussian splatting." << std::endl;
            return;
        }

        osgVerse::ResourceManager* res = osgVerse::ResourceManager::instance();
        vert->setName("Gaussian_VS"); geom->setName("Gaussian_GS"); frag->setName("Gaussian_FS");
        res->shareShader(vert, true); res->shareShader(geom, true); res->shareShader(frag, true);

        osgVerse::GaussianGeometry::RenderMethod method = osgVerse::GaussianGeometry::INSTANCING;
        if (hint == "TBO") method = osgVerse::GaussianGeometry::INSTANCING_TEXTURE;
        else if (hint == "TEX2D") method = osgVerse::GaussianGeometry::INSTANCING_TEX2D;
        else if (hint == "GS") method = osgVerse::GaussianGeometry::GEOMETRY_SHADER;
        _program = osgVerse::GaussianGeometry::createProgram(vert, (hint == "GS") ? geom : NULL, frag, method);
        _callback = osgVerse::GaussianGeometry::createUniformCallback();

        // FIXME: it seems import_defines failed in GLCore/GLES mode? Try ShaderLibrary as fallback
        std::vector<std::string> gsDefinitions;
        if (hint == "TBO")
            vert->setUserValue("Definitions", std::string("USE_INSTANCING,USE_INSTANCING_TEX"));
        else if (hint == "TEX2D")
            vert->setUserValue("Definitions", std::string("USE_INSTANCING,USE_INSTANCING_TEX2D"));
        else if (hint == "GS")
        {

#if defined(OSG_GL3_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
            gsDefinitions.push_back("layout(points) in;");
            gsDefinitions.push_back("layout(triangle_strip, max_vertices = 4) out;");
#endif
        }
        else
            vert->setUserValue("Definitions", std::string("USE_INSTANCING"));

        int cxtVer = 0, glslVer = 0; osgVerse::guessOpenGLVersions(cxtVer, glslVer);
#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
        osgVerse::Pipeline::createShaderDefinitions(vert, cxtVer, glslVer);
#else
        osgVerse::Pipeline::createShaderDefinitions(vert, cxtVer, 430);  // for SSBO under compatible profile
#endif
        osgVerse::Pipeline::createShaderDefinitions(geom, cxtVer, glslVer, gsDefinitions);
        osgVerse::Pipeline::createShaderDefinitions(frag, cxtVer, glslVer);
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
            ss->setAttributeAndModes(new osg::BlendEquation(osg::BlendEquation::FUNC_ADD));
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
        : _sorter(sorter), _firstFrame(true) {}

    virtual void operator()(osg::RenderInfo& renderInfo) const
    {
        if (renderInfo.getCurrentCamera() != NULL) _sorter->cull(renderInfo);
    }

protected:
    osg::ref_ptr<osgVerse::GaussianSorter> _sorter;
    mutable bool _firstFrame;
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

    std::string hint; arguments.read("--render-mode", hint);
    osg::ref_ptr<osgDB::Options> options = new osgDB::Options("RenderMethod=" + hint);
    osg::ref_ptr<osg::Node> gs = osgDB::readNodeFiles(arguments, options.get());
    if (!gs) gs = osgDB::readNodeFile(BASE_DIR + "/models/3dgs_parrot.splat", options.get());
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
    viewer.setRealizeOperation(new osgVerse::RealizeOperation);

    osgVerse::QuickEventHandler* handler = new osgVerse::QuickEventHandler;
    handler->addKeyUpCallback('1', [&](int key) { root->getStateSet()->getUniform("GaussianRenderingMode")->set(1.0f); });
    handler->addKeyUpCallback('0', [&](int key) { root->getStateSet()->getUniform("GaussianRenderingMode")->set(0.0f); });
    viewer.addEventHandler(handler);

    osg::ref_ptr<osgVerse::GaussianSorter> sorter = new osgVerse::GaussianSorter;
    GaussianStateVisitor gsv(sorter.get(), hint); gs->accept(gsv);
    viewer.getCamera()->setPreDrawCallback(new SortCallback(sorter.get()));

    int screenNo = 0; arguments.read("--screen", screenNo);
    viewer.setUpViewOnSingleScreen(screenNo);
    return viewer.run();
}
