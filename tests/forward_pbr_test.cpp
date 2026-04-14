#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/CullVisitor>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/RenderCallbackXR.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

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

class UpdateHandlerXR : public osgGA::GUIEventHandler
{
public:
    UpdateHandlerXR(osg::StateSet* ss, osgVerse::RenderCallbackXR* xr) : _sceneStateSet(ss), _xr(xr)
    {
        updateStageForStereoVR(osgDB::readShaderFile(
            osg::Shader::GEOMETRY, SHADER_DIR + "std_forward_render.geom.glsl"), true);
    }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (_xr.valid())
        {
            osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
            if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
            {
                osg::Matrix vMatrix, pMatrix;
                osg::Camera* cam = view->getCamera();

                _xr->handleEvents(view->getEventQueue());
                if (updateMatricesForStereoVR(vMatrix, pMatrix, cam->getProjectionMatrix()))
                {
                    // Also apply to main camera
                    osgGA::CameraManipulator* mani = view->getCameraManipulator();
                    if (mani) mani->setByInverseMatrix(vMatrix);
                    else cam->setViewMatrix(vMatrix);
                    cam->setProjectionMatrix(pMatrix);
                }
            }
        }
        return false;
    }

    void updateStageForStereoVR(osg::Shader* geomShader, bool useClip)
    {
        int cxtVer = 0, glslVer = 0; osgVerse::guessOpenGLVersions(cxtVer, glslVer);
        osgVerse::Pipeline::createShaderDefinitions(geomShader, cxtVer, glslVer);

        osg::Program* prog = static_cast<osg::Program*>(
            _sceneStateSet->getAttribute(osg::StateAttribute::PROGRAM));
        if (prog && prog->getParameter(GL_GEOMETRY_VERTICES_OUT_EXT) <= 3)
        {
#if OSG_VERSION_GREATER_THAN(3, 3, 6)
            //prog->addDefinitions(osg::Shader::VERTEX, "#define VERSE_VRMODE 1");
            _sceneStateSet->setDefine("VERSE_VRMODE");
#endif
            prog->addShader(geomShader);
            prog->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 2 * 3);  // Left/Right
            prog->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
            prog->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLES);
        }

        if (useClip)
        {
#if !defined(OSG_GL3_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
            _sceneStateSet->setMode(GL_CLIP_PLANE0, osg::StateAttribute::ON);
            _sceneStateSet->setMode(GL_CLIP_PLANE1, osg::StateAttribute::ON);
            _sceneStateSet->setMode(GL_CLIP_PLANE2, osg::StateAttribute::ON);
            _sceneStateSet->setMode(GL_CLIP_PLANE3, osg::StateAttribute::ON);
#endif
        }
    }

    bool updateMatricesForStereoVR(osg::Matrix& view, osg::Matrix& proj, const osg::Matrix& proj0)
    {
        osg::Matrixf viewL, viewR, projL, projR;
        double znear = 0.0, zfar = 0.0; extractNearFar(proj0, znear, zfar);
        if (_xr->begin(viewL, viewR, projL, projR, znear, zfar))
        {
            osg::Uniform* v = _sceneStateSet->getOrCreateUniform("viewMatrices", osg::Uniform::FLOAT_MAT4, 2);
            osg::Uniform* p = _sceneStateSet->getOrCreateUniform("projMatrices", osg::Uniform::FLOAT_MAT4, 2);
            v->setElement(0, viewL); v->setElement(1, viewR); p->setElement(0, projL); p->setElement(1, projR);
            view = viewL; proj = projL; return true;
        }
        return false;
    }

    void extractNearFar(const osg::Matrix& proj, double& znear, double& zfar)
    {
        double A = proj(2, 2), B = proj(3, 2);
        znear = B / (A - 1.0); zfar = B / (A + 1.0);
    }

protected:
    osg::observer_ptr<osg::StateSet> _sceneStateSet;
    osg::observer_ptr<osgVerse::RenderCallbackXR> _xr;
};

osg::StateSet* createPbrStateSet(osgVerse::Pipeline* pipeline)
{
    osg::ref_ptr<osg::StateSet> forwardSS = pipeline->createForwardStateSet(
        osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "std_forward_render.vert.glsl"),
        osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "std_forward_render.frag.glsl"));

    osgVerse::LightModule* lm = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (forwardSS.valid() && lm)
    {
        forwardSS->setTextureAttributeAndModes(7, lm->getParameterTable());
        forwardSS->addUniform(new osg::Uniform("LightParameterMap", 7));
        forwardSS->addUniform(lm->getLightNumber());
    }
    return forwardSS.release();
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);

    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments);
    if (!scene) scene = osgDB::readNodeFile(BASE_DIR + "/models/Sponza/Sponza.gltf.125,125,125.scale");
    if (!scene) { OSG_WARN << "Failed to load scene model"; return 1; }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());

    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(1.0f, 1.0f, 1.0f));
    light0->setDirection(osg::Vec3(0.02f, 0.1f, -1.0f));
    light0->setDirectional(true);

    osg::ref_ptr<osgVerse::LightDrawable> light1 = new osgVerse::LightDrawable;
    light1->setColor(osg::Vec3(1.0f, 1.0f, 1.0f));
    light1->setDirection(osg::Vec3(-0.4f, 0.6f, 0.1f));
    light1->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());  // Main light
    lightGeode->addDrawable(light1.get());

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());
    root->addChild(lightGeode.get());

    // The pipeline only for shader construction and lighting
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    osg::ref_ptr<osgVerse::LightModule> lightModule = new osgVerse::LightModule("Light", pipeline.get());
    lightModule->setMainLight(light0.get(), "");  // no shadow module
    sceneRoot->setStateSet(createPbrStateSet(pipeline.get()));

    // Start the viewer
    osgViewer::Viewer viewer;
    viewer.getCamera()->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    viewer.getCamera()->addUpdateCallback(lightModule.get());
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setRealizeOperation(new osgVerse::RealizeOperation);

    if (arguments.read("--openxr"))
    {
        // Optional, VR mode
        osgVerse::RenderCallbackXR* xr = new osgVerse::RenderCallbackXR;
        xr->setup(viewer.getCamera(), 2);  // post-draw callback to endFrame() and submit frame buffer to VR
        viewer.addEventHandler(new UpdateHandlerXR(sceneRoot->getStateSet(), xr));  // frame event to beginFrame()

        // Move the model far from the HMD position to make it visible at first
        sceneRoot->setMatrix(osg::Matrix::translate(0.0f, 0.0f, -scene->getBound().radius()));
    }

    int screenNo = 0; arguments.read("--screen", screenNo);
    viewer.setUpViewOnSingleScreen(screenNo);
    return viewer.run();
}
