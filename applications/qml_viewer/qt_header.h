#ifndef MANA_APP_QMLVIEWER_HPP
#define MANA_APP_QMLVIEWER_HPP

#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtOpenGL/QtOpenGL>
#ifdef USE_QT6
#   include <QtOpenGLWidgets/QtOpenGLWidgets>
#endif
#include <QtQuick/QtQuick>
#include <QtQml/QtQml>
#include <QOpenGLFunctions_3_3_Core>

#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osgUtil/Statistics>
#include <osgViewer/Renderer>
#include <osgViewer/CompositeViewer>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>

class MyView : public osgViewer::View
{
public:
    MyView(osgVerse::Pipeline* p) : osgViewer::View(), _pipeline(p) {}
    osg::ref_ptr<osgVerse::Pipeline> _pipeline;

    typedef std::vector<osg::Camera*> Cameras;
    typedef std::vector<osg::GraphicsContext*> Contexts;
    typedef std::vector<osgViewer::Scene*> Scenes;

    void getCameras(Cameras& cameras, bool onlyActive = true)
    {
        cameras.clear();
        if (getCamera() && (!onlyActive || (getCamera()->getGraphicsContext() &&
                            getCamera()->getGraphicsContext()->valid()))) cameras.push_back(getCamera());
        for (osgViewer::View::Slaves::iterator itr = _slaves.begin(); itr != _slaves.end(); ++itr)
        {
            if (itr->_camera.valid() && (!onlyActive || (itr->_camera->getGraphicsContext() &&
                                         itr->_camera->getGraphicsContext()->valid()))) cameras.push_back(itr->_camera.get());
        }
    }

    void getContexts(Contexts& contexts, bool onlyValid = true)
    {
        typedef std::set<osg::GraphicsContext*> ContextSet;
        ContextSet contextSet; contexts.clear();

        osg::GraphicsContext* gc = getCamera() ? getCamera()->getGraphicsContext() : 0;
        if (gc && (gc->valid() || !onlyValid))
        {
            if (contextSet.count(gc) == 0)
            { contextSet.insert(gc); contexts.push_back(gc); }
        }

        for (unsigned int i = 0; i < getNumSlaves(); ++i)
        {
            osgViewer::View::Slave& slave = getSlave(i);
            osg::GraphicsContext* sgc = slave._camera.valid() ? slave._camera->getGraphicsContext() : 0;
            if (sgc && (sgc->valid() || !onlyValid))
            {
                if (contextSet.count(sgc) == 0)
                { contextSet.insert(sgc); contexts.push_back(sgc); }
            }
        }
    }

    void getScenes(Scenes& scenes, bool onlyValid = true)
    {
        typedef std::set<osgViewer::Scene*> SceneSet;
        SceneSet sceneSet; scenes.clear();
        if (getScene() && (!onlyValid || getScene()->getSceneData()))
        {
            if (sceneSet.count(getScene()) == 0)
            { sceneSet.insert(getScene()); scenes.push_back(getScene()); }
        }
    }

protected:
    virtual osg::GraphicsOperation* createRenderer(osg::Camera* camera)
    {
        if (_pipeline.valid()) return _pipeline->createRenderer(camera);
        else return osgViewer::View::createRenderer(camera);
    }
};

class MyCompositeViewer : public osgViewer::CompositeViewer
{
public:
    void frameNoRendering(double simulationTime = USE_REFERENCE_TIME)
    {
        if (_done) return;
        if (_firstFrame)
        {
            viewerInit();
            if (!isRealized()) realize();
            _firstFrame = false;
        }

        advance(simulationTime);
        eventTraversal();
        updateTraversal();
        //renderingTraversals();  // no rendering!
    }

    void renderView(MyView* view)
    {
        double beginRenderingTraversals = elapsedTime();
        Contexts contexts; view->getContexts(contexts);
        checkWindowStatus(contexts); if (_done) return;

        osg::FrameStamp* frameStamp = getViewerFrameStamp();
        unsigned int frameNumber = frameStamp ? frameStamp->getFrameNumber() : 0;
        if (getViewerStats() && getViewerStats()->collectStats("scene"))
        {
            osg::Stats* stats = view->getStats();
            osg::Node* sceneRoot = view->getSceneData();
            if (sceneRoot && stats)
            {
                osgUtil::StatsVisitor statsVisitor;
                sceneRoot->accept(statsVisitor); statsVisitor.totalUpStats();

                unsigned int unique_primitives = 0;
                osgUtil::Statistics::PrimitiveCountMap::iterator pcmitr;
                for (pcmitr = statsVisitor._uniqueStats.GetPrimitivesBegin();
                     pcmitr != statsVisitor._uniqueStats.GetPrimitivesEnd(); ++pcmitr)
                { unique_primitives += pcmitr->second; }

                stats->setAttribute(frameNumber, "Number of unique StateSet", static_cast<double>(statsVisitor._statesetSet.size()));
                stats->setAttribute(frameNumber, "Number of unique Group", static_cast<double>(statsVisitor._groupSet.size()));
                stats->setAttribute(frameNumber, "Number of unique Transform", static_cast<double>(statsVisitor._transformSet.size()));
                stats->setAttribute(frameNumber, "Number of unique LOD", static_cast<double>(statsVisitor._lodSet.size()));
                stats->setAttribute(frameNumber, "Number of unique Switch", static_cast<double>(statsVisitor._switchSet.size()));
                stats->setAttribute(frameNumber, "Number of unique Geode", static_cast<double>(statsVisitor._geodeSet.size()));
                stats->setAttribute(frameNumber, "Number of unique Drawable", static_cast<double>(statsVisitor._drawableSet.size()));
                stats->setAttribute(frameNumber, "Number of unique Geometry", static_cast<double>(statsVisitor._geometrySet.size()));
                stats->setAttribute(frameNumber, "Number of unique Vertices", static_cast<double>(statsVisitor._uniqueStats._vertexCount));
                stats->setAttribute(frameNumber, "Number of unique Primitives", static_cast<double>(unique_primitives));

                unsigned int instanced_primitives = 0;
                for (pcmitr = statsVisitor._instancedStats.GetPrimitivesBegin();
                     pcmitr != statsVisitor._instancedStats.GetPrimitivesEnd(); ++pcmitr)
                { instanced_primitives += pcmitr->second; }

                stats->setAttribute(frameNumber, "Number of instanced Stateset", static_cast<double>(statsVisitor._numInstancedStateSet));
                stats->setAttribute(frameNumber, "Number of instanced Group", static_cast<double>(statsVisitor._numInstancedGroup));
                stats->setAttribute(frameNumber, "Number of instanced Transform", static_cast<double>(statsVisitor._numInstancedTransform));
                stats->setAttribute(frameNumber, "Number of instanced LOD", static_cast<double>(statsVisitor._numInstancedLOD));
                stats->setAttribute(frameNumber, "Number of instanced Switch", static_cast<double>(statsVisitor._numInstancedSwitch));
                stats->setAttribute(frameNumber, "Number of instanced Geode", static_cast<double>(statsVisitor._numInstancedGeode));
                stats->setAttribute(frameNumber, "Number of instanced Drawable", static_cast<double>(statsVisitor._numInstancedDrawable));
                stats->setAttribute(frameNumber, "Number of instanced Geometry", static_cast<double>(statsVisitor._numInstancedGeometry));
                stats->setAttribute(frameNumber, "Number of instanced Vertices", static_cast<double>(statsVisitor._instancedStats._vertexCount));
                stats->setAttribute(frameNumber, "Number of instanced Primitives", static_cast<double>(instanced_primitives));
            }
        }

        Scenes scenes; view->getScenes(scenes);
        for (Scenes::iterator sitr = scenes.begin(); sitr != scenes.end(); ++sitr)
        {
            osgViewer::Scene* scene = *sitr; if (!scene) continue;
            osgDB::DatabasePager* dp = scene->getDatabasePager();
            if (dp) dp->signalBeginFrame(frameStamp);

            osgDB::ImagePager* ip = scene->getImagePager();
            if (ip) ip->signalBeginFrame(frameStamp);
            if (scene->getSceneData()) scene->getSceneData()->getBound();
        }

        bool doneMakeCurrentInThisThread = false;
        if (_endDynamicDrawBlock.valid()) _endDynamicDrawBlock->reset();
        if (_startRenderingBarrier.valid()) _startRenderingBarrier->block();

        // reset any double buffer graphics objects
        Cameras cameras; view->getCameras(cameras);
        for (Cameras::iterator camItr = cameras.begin(); camItr != cameras.end(); ++camItr)
        {
            osg::Camera* camera = *camItr;
            osgViewer::Renderer* r = dynamic_cast<osgViewer::Renderer*>(camera->getRenderer()); if (!r) continue;
            if (!r->getGraphicsThreadDoesCull() && !(camera->getCameraThread())) r->cull();
        }

        Contexts::iterator itr;
        for (itr = contexts.begin(); itr != contexts.end() && !_done; ++itr)
        {
            if (!((*itr)->getGraphicsThread()) && (*itr)->valid())
            {
                doneMakeCurrentInThisThread = true;
                makeCurrent(*itr); (*itr)->runOperations();
            }
        }

        if (_endRenderingDispatchBarrier.valid()) _endRenderingDispatchBarrier->block();
        for (itr = contexts.begin(); itr != contexts.end() && !_done; ++itr)
        {
            if (!((*itr)->getGraphicsThread()) && (*itr)->valid())
            {
                doneMakeCurrentInThisThread = true;
                makeCurrent(*itr); (*itr)->swapBuffers();
            }
        }

        for (Scenes::iterator sitr = scenes.begin(); sitr != scenes.end(); ++sitr)
        {
            osgViewer::Scene* scene = *sitr; if (!scene) continue;
            osgDB::DatabasePager* dp = scene->getDatabasePager();
            if (dp) dp->signalEndFrame();

            osgDB::ImagePager* ip = scene->getImagePager();
            if (ip) ip->signalEndFrame();
        }

        if (_endDynamicDrawBlock.valid()) _endDynamicDrawBlock->block();
        if (_releaseContextAtEndOfFrameHint && doneMakeCurrentInThisThread) releaseContext();
        if (getViewerStats() && getViewerStats()->collectStats("update"))
        {
            double endRenderingTraversals = elapsedTime();  // update current frames stats
            getViewerStats()->setAttribute(frameNumber, "Rendering traversals begin time ", beginRenderingTraversals);
            getViewerStats()->setAttribute(frameNumber, "Rendering traversals end time ", endRenderingTraversals);
            getViewerStats()->setAttribute(frameNumber, "Rendering traversals time taken", endRenderingTraversals - beginRenderingTraversals);
        }
        _requestRedraw = false;
    }
};

class OsgFramebufferObjectRenderer : public QQuickFramebufferObject::Renderer
{
public:
    OsgFramebufferObjectRenderer(const QQuickFramebufferObject* fbo) : _fboParentItem(fbo) {}
    virtual QOpenGLFramebufferObject* createFramebufferObject(const QSize &size);
    virtual void render();

protected:
    const QQuickFramebufferObject* _fboParentItem;
};

class OsgFramebufferObject : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(QString renderMode READ getRenderMode
               WRITE setRenderMode
               NOTIFY renderModeChanged)

public:
    OsgFramebufferObject(QQuickItem* parent = NULL);

    void setParentViewer(MyCompositeViewer* v) { _parentViewer = v; }
    MyCompositeViewer* getParentViewer() const { return _parentViewer; }

    virtual QQuickFramebufferObject::Renderer* createRenderer() const
    { return new OsgFramebufferObjectRenderer(this); }

    virtual void initializeScene(bool pbrMode);
    osgViewer::GraphicsWindow* getGraphicsWindow() const { return _graphicsWindow.get(); }
    MyView* getView() const { return _view.get(); }

    QString getRenderMode() const { return _renderMode; }
    void setRenderMode(const QString& name) { _renderMode = name; }

signals:
    void renderModeChanged(const QString& name);

protected:
    virtual void componentComplete();
#ifdef USE_QT6
    virtual void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry);
#else
    virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry);
#endif
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void hoverMoveEvent(QHoverEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _graphicsWindow;
    osg::ref_ptr<MyView> _view;
    MyCompositeViewer* _parentViewer;

    osgVerse::StandardPipelineParameters _params;
    QTimer _updateTimer;
    QString _renderMode;
    int _lastModifiers;
};

#endif
