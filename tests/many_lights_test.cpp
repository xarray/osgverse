#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Geode>
#include <osg/PolygonMode>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osg/FrameStamp>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgText/Text>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/LightCluster.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <random>
#include <sstream>

//#include <microprofile.h>
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

namespace
{
    /** Create N random point/spot lights covering the scene bounds. They are just LightDrawable
        objects added to a Geode (exactly like the single main light), so that the existing
        culling callback of the light module collects them; no per-light shadow is needed here */
    void createRandomLights(osg::Geode* lightRoot, const osg::BoundingBox& bb, int numLights,
                            float rangeScale, float powerScale, float spotRatio, unsigned int seed)
    {
        std::mt19937 gen(seed);
        std::uniform_real_distribution<float> rnd(0.0f, 1.0f);
        osg::Vec3 ext = bb._max - bb._min;

        // Shrink the placement volume a bit so the lights stay inside the scene
        osg::Vec3 lo = bb._min + ext * 0.15f, hi = bb._max - ext * 0.15f;
        float maxExt = osg::maximum(ext.x(), osg::maximum(ext.y(), ext.z()));
        float baseRange = maxExt * rangeScale;

        for (int i = 0; i < numLights; ++i)
        {
            osg::ref_ptr<osgVerse::LightDrawable> light = new osgVerse::LightDrawable;
            osg::Vec3 pos(lo.x() + (hi.x() - lo.x()) * rnd(gen),
                          lo.y() + (hi.y() - lo.y()) * rnd(gen),
                          lo.z() + (hi.z() - lo.z()) * rnd(gen));
            light->setPosition(pos);

            float range = baseRange * (0.6f + 0.8f * rnd(gen));
            light->setRange(range);

            // The shader attenuates a light with an inverse-square term, so the power has to grow
            // with range * range for the lights to stay visible when the scene scale changes
            osg::Vec3 color(rnd(gen), rnd(gen), rnd(gen));
            color.normalize();
            color = color * (range * range * powerScale * (0.5f + 1.5f * rnd(gen)));
            light->setColor(color);

            osg::Vec3 dir(rnd(gen) - 0.5f, rnd(gen) - 0.5f, -(0.5f + rnd(gen)));  // mostly downward
            light->setDirection(dir);
            if (rnd(gen) < spotRatio) light->setSpotCutoff(0.35f + 0.55f * rnd(gen));  // radians
            lightRoot->addDrawable(light.get());
        }
    }

    /** Show the LightDrawable debug geometries as a wireframe overlay: a sphere for a point light,
        a cuboid for a directional light and a cone for a spot light (see
        LightDrawable::recreate()). It is meant for inspecting where the lights of a scene are */
    void showLightGeometries(osg::Geode* lightRoot, bool on, osg::PolygonMode* mode)
    {
        if (lightRoot == NULL) return;
        for (unsigned int i = 0; i < lightRoot->getNumDrawables(); ++i)
        {
            osgVerse::LightDrawable* ld =
                dynamic_cast<osgVerse::LightDrawable*>(lightRoot->getDrawable(i));
            if (ld != NULL) ld->setDebugShow(on);
        }

        osg::StateSet* ss = lightRoot->getOrCreateStateSet();
        if (on)
            ss->setAttributeAndModes(mode, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        else
            ss->removeAttribute(osg::StateAttribute::POLYGONMODE);
    }

    /** Simple on-screen text HUD showing the light count and the state of the light clustering */
    class LightStatHandler : public osgVerse::QuickEventHandler
    {
    public:
        LightStatHandler(osgVerse::LightModule* m, osgText::Text* text, int numLights)
            : _module(m), _text(text), _numLights(numLights), _animated(true),
              _showDebug(false), _frame(0), _reported(false)
        {
            _polygonMode = new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,
                                                osg::PolygonMode::LINE);
            addKeyUpCallback('c', [this](int) { toggleCluster(); });
            addKeyUpCallback('C', [this](int) { toggleCluster(); });
            addKeyUpCallback('a', [this](int) { _animated = !_animated; });
            addKeyUpCallback('A', [this](int) { _animated = !_animated; });
            addKeyUpCallback('l', [this](int) { toggleDebug(); });
            addKeyUpCallback('L', [this](int) { toggleDebug(); });
        }

        void setLightRoot(osg::Geode* g) { _lightRoot = g; }
        bool getShowDebug() const { return _showDebug; }
        void setShowDebug(bool b) { _showDebug = b; showLightGeometries(_lightRoot.get(), b, _polygonMode.get()); }

        bool getAnimated() const { return _animated; }
        void setAnimated(bool b) { _animated = b; }

        virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
        {
            if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
            {
                _frame++;
                if ((_frame % 15) == 0) updateText();
                if (_frame == 30 && _module.valid())
                {   // Report once how many lights really reached the light table, which is the
                    // number the shader loops over in its flat (non-clustered) path
                    osg::Vec2 num(0.0f, 0.0f); _module->getLightNumber()->get(num);
                    OSG_NOTICE << "[ManyLights] " << (int)num.x() << " lights are active in the "
                               << "table (flat-pass cap " << (int)num.y() << ")" << std::endl;
                    osg::View* view = dynamic_cast<osg::View*>(&aa);
                    if (view && view->getCamera())
                    {
                        osg::Vec3d eye, center, up;
                        view->getCamera()->getViewMatrixAsLookAt(eye, center, up);
                        OSG_NOTICE << "[ManyLights] Camera eye: (" << eye.x() << " " << eye.y() << " "
                                   << eye.z() << "), look-at: (" << center.x() << " " << center.y()
                                   << " " << center.z() << ")" << std::endl;
                    }
                    _reported = true;
                }
            }
            return osgVerse::QuickEventHandler::handle(ea, aa);
        }

        void updateText()
        {
            std::stringstream ss; ss << "Lights: " << _numLights;
            if (_module.valid())
            {
                osg::Vec2 num(0.0f, 0.0f); _module->getLightNumber()->get(num);
                const osgVerse::LightCluster* cluster = _module->getLightCluster();
                ss << " (active: " << (int)num.x() << ")"
                   << "\nLight cluster: " << (_module->getClusterEnabled() ? "ON" : "OFF");
                if (_module->getClusterEnabled() && cluster != NULL)
                {
                    const osg::Vec3i& g = cluster->getGridSize();
                    ss << "\nGrid: " << g.x() << "x" << g.y() << "x" << g.z() << " = "
                       << cluster->getNumClusters() << " clusters, " << cluster->getNumIndexEntries()
                       << " light refs";
                }
            }
            ss << "\nAnimation: " << (_animated ? "ON" : "OFF")
               << ", light wireframe: " << (_showDebug ? "ON" : "OFF")
               << "\nKeys: [C] cluster, [A] animation, [L] light wireframe";
            if (_text.valid())
            {
                _text->setText(ss.str());
                _text->dirtyDisplayList();
            }
        }

    protected:
        void toggleCluster()
        {
            if (_module.valid()) _module->setClusterEnabled(!_module->getClusterEnabled());
            updateText();
        }

        void toggleDebug() { setShowDebug(!_showDebug); updateText(); }

        osg::observer_ptr<osgVerse::LightModule> _module;
        osg::observer_ptr<osgText::Text> _text;
        osg::observer_ptr<osg::Geode> _lightRoot;
        osg::ref_ptr<osg::PolygonMode> _polygonMode;
        int _numLights; bool _animated, _showDebug, _reported; int _frame;
    };
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);

    int numLights = 128; arguments.read("--lights", numLights);
    // The attenuation range is taken as a fraction of the scene size, so that the light pools stay
    // small compared to the scene (a range of the same order as the scene itself would light
    // everything and blow the near surfaces out). The power follows range * range because the
    // shader attenuates with an inverse-square term, which keeps a light well exposed at the
    // distance of its own range whatever the scene scale is
    float rangeScale = 0.12f; arguments.read("--light-range", rangeScale);
    float powerScale = 0.15f; arguments.read("--light-power", powerScale);
    float spotRatio = 0.35f; arguments.read("--spot-ratio", spotRatio);
    bool animate = !arguments.read("--no-animate");
    bool startNoCluster = arguments.read("--no-cluster");
    bool noSsao = arguments.read("--no-ssao");
    bool noBloom = arguments.read("--no-bloom");
    bool showLights = arguments.read("--show-lights");
    unsigned seed = 1234; arguments.read("--seed", seed);
    int clusterTile = 128; arguments.read("--cluster-tile", clusterTile);
    int clusterSlices = 16; arguments.read("--cluster-slices", clusterSlices);

    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments);
    if (!scene) scene = osgDB::readNodeFile(BASE_DIR + "/models/Sponza.osgb");
    if (!scene) { OSG_WARN << "Failed to load any scene" << std::endl; return 1; }

    osg::ComputeBoundsVisitor cbv; scene->accept(cbv);
    osg::BoundingBox bb = cbv.getBoundingBox();
    if (!bb.valid()) { OSG_WARN << "Invalid scene bounds" << std::endl; return 1; }
    OSG_NOTICE << "[ManyLights] Scene bounds: (" << bb.xMin() << " " << bb.yMin() << " " << bb.zMin()
               << ") - (" << bb.xMax() << " " << bb.yMax() << " " << bb.zMax() << ")" << std::endl;

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());

    // HUD text
    osg::ref_ptr<osgText::Text> hudText = new osgText::Text;
    hudText->setPosition(osg::Vec3(20.0f, 1060.0f, 0.0f));
    hudText->setAlignment(osgText::Text::LEFT_TOP);
    hudText->setCharacterSize(18.0f);
    hudText->setColor(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    if (osgDB::fileExists(MISC_DIR + "LXGWFasmartGothic.ttf"))
        hudText->setFont(MISC_DIR + "LXGWFasmartGothic.ttf");

    osg::ref_ptr<osg::Camera> hudCamera = osgVerse::createHUDCamera(NULL, 1920, 1080);
    osgVerse::Pipeline::setPipelineMask(*hudCamera, FORWARD_SCENE_MASK);
    { osg::ref_ptr<osg::Geode> g = new osg::Geode; g->addDrawable(hudText.get()); hudCamera->addChild(g.get()); }
    root->addChild(hudCamera.get());

    // Start the viewer with the standard (deferred + post effects) pipeline
    osg::ref_ptr<osgVerse::StandardPipelineViewer> viewer =
        new osgVerse::StandardPipelineViewer(true, false, false);
    viewer->getParameters().enableAO = !noSsao;  // SSAO uses a random kernel (noisy in captures)
    viewer->getParameters().enableBloom = !noBloom;  // bloom also drives the eye adaptation
    viewer->getParameters().shadowNumber = 0;  // no shadow needed: the lights are not shadow casters
    viewer->setSceneData(root.get());
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->addEventHandler(new osgViewer::WindowSizeHandler);
    viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->setUpViewOnSingleScreen(0);
    viewer->realize();

    // Configure the light module: TBO is the default, cluster culling is on by default
    osgVerse::Pipeline* pipeline = viewer->getPipeline();
    osgVerse::LightModule* lightModule =
        static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (lightModule == NULL) { OSG_WARN << "No light module found" << std::endl; return 1; }
    // The flat loop can show them all too. Note the +1: the main directional light already lives in
    // the light geode, so the table holds numLights + 1 entries, and a cap of numLights would make
    // the non-clustered path silently drop one light (the clustered one keeps it), which shows up
    // as a difference between the two paths as soon as that light lights something
    lightModule->setMaxLightsInPass(osg::maximum(24, numLights + 1));
    lightModule->setClusterEnabled(!startNoCluster);
    if (lightModule->getLightCluster())
    {
        lightModule->getLightCluster()->setTileSize(clusterTile);
        lightModule->getLightCluster()->setNumSlices(clusterSlices);
    }

    // The scene stays framed by the manipulator home position (a natural view of the whole scene);
    // no lateral or corner offset is applied. The agent options can still override the view
    osg::Geode* lightRoot = viewer->getLightRoot();

    // The global directional light is dimmed to a fill level (it can even be turned off): the
    // point/spot lights created below are the subject of this example. A small range is given to
    // it so that its debug cuboid has a visible size when --show-lights is used; the negligible
    // color keeps it from lighting the scene or casting a shadow
    if (lightModule->getMainLight() != NULL)
    {
        osg::Vec3 ext = bb._max - bb._min;
        lightModule->getMainLight()->setColor(osg::Vec3(0.02f, 0.02f, 0.02f));
        lightModule->getMainLight()->setRange(
            osg::maximum(ext.x(), osg::maximum(ext.y(), ext.z())) * 0.15f);
    }

    // Place the many lights in the existing light geode (index 0 is the main directional light)
    createRandomLights(lightRoot, bb, numLights, rangeScale, powerScale, spotRatio, seed);

    // HUD handler owns the text content and the hot keys, including the light wireframe toggle
    osg::ref_ptr<LightStatHandler> statHandler =
        new LightStatHandler(lightModule, hudText.get(), numLights);
    statHandler->setLightRoot(lightRoot);
    statHandler->setAnimated(animate);
    statHandler->setShowDebug(showLights);
    statHandler->updateText();
    viewer->addEventHandler(statHandler.get());
    if (showLights)
    {
        OSG_NOTICE << "[ManyLights] Light debug geometries are shown as a wireframe: sphere = "
                   << "point light, cuboid = directional light, cone = spot light" << std::endl;
    }

    // Agent automation (optional): set the view, capture and quit. This is a no-op unless one of
    // the --agent* options is given (see configureViewerForAgent in pipeline/Utilities.h)
    osgVerse::configureViewerForAgent(arguments, viewer.get());

    std::cout << "Many-lights testing started (num=" << numLights << ", cluster="
              << (lightModule->getClusterEnabled() ? "on" : "off") << ")..." << std::endl;

    osg::ref_ptr<osg::FrameStamp> lastStamp;
    double time0 = 0.0;
    while (!viewer->done())
    {
        if (statHandler->getAnimated())
        {
            if (time0 == 0.0) time0 = viewer->getFrameStamp()->getSimulationTime();
            double t = viewer->getFrameStamp()->getSimulationTime() - time0;

            // Move the extra lights (index 0 is the main light, which the manipulator/viewer owns)
            for (unsigned int i = 1; i < lightRoot->getNumDrawables(); ++i)
            {
                osgVerse::LightDrawable* ld =
                    dynamic_cast<osgVerse::LightDrawable*>(lightRoot->getDrawable(i));
                if (!ld) continue;
                osg::Vec3 p = ld->getPosition();
                p.x() += 0.02f * (float)cos(t * 1.7 + i);
                p.y() += 0.02f * (float)sin(t * 1.3 + i * 2);
                ld->setPosition(p);
            }
        }
        viewer->frame();
    }
    return 0;
}
