#include <osg/io_utils>
#include <osg/ValueObject>
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

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
#endif
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)
USE_GRAPICSWINDOW_IMPLEMENTATION(GLFW)

class GaussianStateVisitor : public osg::NodeVisitor
{
public:
    GaussianStateVisitor(osgVerse::GaussianSorter* s, const std::string& hint, bool testColor)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN), _sorter(s), _testColorCustomizing(testColor)
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

        std::vector<std::string> gsDefinitions; int minGlslVer = 120;
        std::string vsDefinitions; if (_testColorCustomizing) vsDefinitions = "CUSTOMIZED_TEX,";
        if (hint == "TBO") vsDefinitions += std::string("USE_INSTANCING,USE_INSTANCING_TEX");
        else if (hint == "TEX2D") vsDefinitions += std::string("USE_INSTANCING,USE_INSTANCING_TEX2D");
        else if (hint == "GS")
        {
#if defined(OSG_GL3_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
            gsDefinitions.push_back("layout(points) in;");
            gsDefinitions.push_back("layout(triangle_strip, max_vertices = 4) out;");
#endif
        }
        else
        {
            vsDefinitions += std::string("USE_INSTANCING");
            minGlslVer = 430;  // for SSBO compatibility
        }

        // FIXME: it seems import_defines failed in GLCore/GLES mode? Try using ShaderLibrary as fallback
        if (!vsDefinitions.empty()) vert->setUserValue("Definitions", vsDefinitions);

        int cxtVer = 0, glslVer = 0; osgVerse::guessOpenGLVersions(cxtVer, glslVer);
        glslVer = osg::maximum(glslVer, minGlslVer);
        osgVerse::Pipeline::createShaderDefinitions(vert, cxtVer, glslVer);
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

                if (_testColorCustomizing)
                {
                    std::vector<osg::Vec4> param(gs->getNumSplats(), osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
                    for (size_t i = 0; i < param.size() / 2; ++i)
                        param[i] = osg::Vec4(1.0f, 0.0f, 0.0f, 2.0f);  // a = 1: replacing; a = 2: additive
                    gs->setColorParameters(param);
                }
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
    bool _testColorCustomizing;
};

struct CudaRadixSorter : public osgVerse::GaussianSorter::UserCallback
{
    CudaRadixSorter() { _context = osgVerse::CudaAlgorithm::initializeContext(0); }
    ~CudaRadixSorter() { osgVerse::CudaAlgorithm::deinitializeContext(_context); }
    CUcontext _context;

    virtual bool sort(osg::VectorGLuint* indices, osg::Vec3* pos, size_t numSplats,
                      const osg::Matrix& model, const osg::Matrix& view)
    {
        osg::Matrix localToEye = model * view; size_t size = indices->size();
        std::vector<unsigned int> inValues(size), outIDs(size);
        std::vector<unsigned int> inIDs(indices->begin(), indices->end());

        for (size_t i = 0; i < size; ++i)
        {   // comparing floating-point numbers as integers
            float d = (pos[(*indices)[i]] * localToEye).z();
            union { float f; uint32_t u; } un = { (d > 0.0f ? 0.0f : (-d)) };
            inValues[i] = (GLuint)un.u; //if (d > 0.0f) numCulled++;
        }

        if (osgVerse::CudaAlgorithm::radixSort(inValues, inIDs, outIDs))
        {
            indices->assign(outIDs.rbegin(), outIDs.rend());
            return true;
        }
        return false;
    }

    virtual bool sort(osg::VectorGLuint* indices, osg::Vec4* pos, size_t numSplats,
                      const osg::Matrix& model, const osg::Matrix& view)
    {
        osg::Matrix localToEye = model * view; size_t size = indices->size();
        std::vector<unsigned int> inValues(size), outIDs(size);
        std::vector<unsigned int> inIDs(indices->begin(), indices->end());

        for (size_t i = 0; i < size; ++i)
        {   // comparing floating-point numbers as integers
            const osg::Vec4& p = pos[(*indices)[i]];
            float d = (osg::Vec3(p[0], p[1], p[2]) * localToEye).z();
            union { float f; uint32_t u; } un = { (d > 0.0f ? 0.0f : (-d)) };
            inValues[i] = (GLuint)un.u; //if (d > 0.0f) numCulled++;
        }

        if (osgVerse::CudaAlgorithm::radixSort(inValues, inIDs, outIDs))
        {
            indices->assign(outIDs.rbegin(), outIDs.rend());
            return true;
        }
        return false;
    }
};

int main(int argc, char** argv)
{
    osgViewer::Viewer viewer;
    viewer.getCamera()->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    viewer.getCamera()->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    viewer.getCamera()->setProjectionMatrixAsPerspective(30.0, 16.0 / 9.0, 0.1, 10000.0);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setRealizeOperation(new osgVerse::RealizeOperation);

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->getOrCreateStateSet()->setMode(GL_DEPTH_CLAMP, osg::StateAttribute::ON);

    osgDB::Registry::instance()->addFileExtensionAlias("ply", "verse_3dgs");
    osgDB::Registry::instance()->addFileExtensionAlias("spz", "verse_3dgs");
    osgDB::Registry::instance()->addFileExtensionAlias("splat", "verse_3dgs");
    osgDB::Registry::instance()->addFileExtensionAlias("ksplat", "verse_3dgs");
    osgDB::Registry::instance()->addFileExtensionAlias("lcc", "verse_3dgs");
    osgDB::Registry::instance()->addFileExtensionAlias("sog", "verse_3dgs");

    osg::ArgumentParser arguments(&argc, argv);
    if (!arguments.read("--custom"))
    {
        // Simplest forward rendering implementation
        osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
        osgVerse::updateOsgBinaryWrappers();

        osg::ref_ptr<osg::Node> gs = osgDB::readNodeFiles(arguments);
        if (!gs) gs = osgDB::readNodeFile(BASE_DIR + "/models/3dgs_parrot.splat");
        if (!gs) { std::cout << "No 3DGS file loaded" << std::endl; return 1; }

        gs->setCullCallback(osgVerse::GaussianGeometry::createUniformCallback());
        root->addChild(gs.get()); viewer.setSceneData(root.get());

        osgVerse::GaussianSorter* sorter = static_cast<osgVerse::GaussianSorter*>(
            osgVerse::getGlobalFileCallback()->getGaussian()->sorterBase.get());
        viewer.getCamera()->setPreDrawCallback(new osgVerse::GaussianSortCallback(sorter));
    }
    else
    {
        osgVerse::globalInitialize(
            argc, argv, osgVerse::defaultInitParameters(osgVerse::NoParameters));  // disable default sorter for test...
        osgVerse::updateOsgBinaryWrappers();

        std::string hint; arguments.read("--render-mode", hint);
        bool testColor = arguments.read("--test-color");
        osg::ref_ptr<osgDB::Options> options = new osgDB::Options("RenderMethod=" + hint);

        osg::ref_ptr<osg::Node> gs = osgDB::readNodeFiles(arguments, options.get());
        if (!gs) gs = osgDB::readNodeFile(BASE_DIR + "/models/3dgs_parrot.splat", options.get());
        if (!gs) { std::cout << "No 3DGS file loaded" << std::endl; return 1; }

        root->getOrCreateStateSet()->getOrCreateUniform("GaussianRenderingMode", osg::Uniform::FLOAT)->set(0.0f);
        root->addChild(gs.get()); viewer.setSceneData(root.get());

        osgVerse::QuickEventHandler* handler = new osgVerse::QuickEventHandler;
        handler->addKeyUpCallback('1', [&](int key) { root->getStateSet()->getUniform("GaussianRenderingMode")->set(1.0f); });
        handler->addKeyUpCallback('0', [&](int key) { root->getStateSet()->getUniform("GaussianRenderingMode")->set(0.0f); });
        viewer.addEventHandler(handler);

        osg::ref_ptr<osgVerse::GaussianSorter> sorter = new osgVerse::GaussianSorter;
        if (arguments.read("--cuda-sort")) sorter->setSortCallback(new CudaRadixSorter);

        GaussianStateVisitor gsv(sorter.get(), hint, testColor); gs->accept(gsv);
        viewer.getCamera()->setPreDrawCallback(new osgVerse::GaussianSortCallback(sorter.get()));
    }

    int screenNo = 0; arguments.read("--screen", screenNo);
    viewer.setUpViewOnSingleScreen(screenNo);
    return viewer.run();
}
