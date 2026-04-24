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
#include <osgGA/FirstPersonManipulator>
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
#include <ui/Utilities.h>
#include <VerseCommon.h>

#ifndef GL_DEPTH_CLAMP
#define GL_DEPTH_CLAMP 0x864F
#endif

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

class QwertyManipulator : public osgGA::FirstPersonManipulator
{
public:
    QwertyManipulator(const osg::Vec3d& e) : osgGA::FirstPersonManipulator(), _presetEye(e)
    { setAllowThrow(false); setVerticalAxisFixed(true); setWheelMovement(0.05f, true); }

    virtual void home(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
    { osgGA::FirstPersonManipulator::home(ea, us); setTransformation(_presetEye, _homeCenter, _homeUp); }

protected:
    virtual bool handleKeyDown(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
    { osgVerse::KeyboardCacher::instance()->advance(ea); return false; }

    virtual bool handleKeyUp(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
    { osgVerse::KeyboardCacher::instance()->advance(ea); return false; }

    virtual bool handleFrame(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
    {
        float m = (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)
                ? (_wheelMovement * _modelSize) : 0.05f;
        osgVerse::KeyboardCacher* kb = osgVerse::KeyboardCacher::instance();
        if (kb->isKeyDown(osgGA::GUIEventAdapter::KEY_Up)) moveForward(m);
        if (kb->isKeyDown(osgGA::GUIEventAdapter::KEY_Down)) moveForward(-m);
        if (kb->isKeyDown(osgGA::GUIEventAdapter::KEY_Left)) moveRight(-m);
        if (kb->isKeyDown(osgGA::GUIEventAdapter::KEY_Right)) moveRight(m);
        return false;
    }

    osg::Vec3d _presetEye;
};

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
#if false
                    osgVerse::PointCloudSegmentation segmentation;
                    segmentation.setPairwiseLinkageFactors(100, osg::PI_2, 1);

                    std::vector<osg::Vec3d> allPoints(gs->getNumSplats());
                    {
                        osg::Vec4* pts4 = gs->getPosition4();
                        if (pts4 != NULL)
                        {
                            for (size_t i = 0; i < allPoints.size(); ++i)
                            { const osg::Vec4& p = pts4[i]; allPoints[i] = osg::Vec3(p[0], p[1], p[2]); }
                        }
                        else
                        {
                            osg::Vec3* pts3 = gs->getPosition3();
                            if (pts3 != NULL) { for (size_t i = 0; i < allPoints.size(); ++i) allPoints[i] = pts3[i]; }
                        }
                    }

                    static osg::Vec3 colors[] = {
                        { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.5f },
                        { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 1.0f }, { 0.5f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.5f }, { 0.5f, 1.0f, 0.5f },
                        { 0.0f, 0.0f, 1.0f }, { 0.5f, 0.0f, 1.0f }, { 0.0f, 0.5f, 1.0f }, { 0.5f, 0.5f, 1.0f }, { 0.5f, 0.5f, 0.5f }
                    };

                    std::vector<std::vector<int>> clusters = segmentation.execute(allPoints);
                    for (size_t i = 0; i < clusters.size(); ++i)
                    {
                        const std::vector<int>& cluster = clusters[i];
                        for (size_t j = 0; j < cluster.size(); ++j)
                            param[cluster[j]] = osg::Vec4(colors[i % 15], 1.0f);  // a = 1: replacing; a = 2: additive
                    }
#else
                    for (size_t i = 0; i < param.size() / 2; ++i)
                        param[i] = osg::Vec4(1.0f, 0.0f, 0.0f, 2.0f);  // a = 1: replacing; a = 2: additive
#endif
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
    std::string savedFile; arguments.read("--save", savedFile);
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

    double firstX = 0.0, firstY = 0.0, firstZ = 0.0;
    if (arguments.read("--first", firstX, firstY, firstZ))
        viewer.setCameraManipulator(new QwertyManipulator(osg::Vec3d(firstX, firstY, firstZ)));
    else
        viewer.setCameraManipulator(new osgGA::TrackballManipulator);

    if (!savedFile.empty()) osgDB::writeNodeFile(*root, savedFile);
    return viewer.run();
}
