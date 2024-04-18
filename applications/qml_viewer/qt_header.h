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
#include <osgViewer/Viewer>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>

class MyViewer : public osgViewer::Viewer
{
public:
    MyViewer(osgVerse::Pipeline* p) : osgViewer::Viewer(), _pipeline(p) {}
    osg::ref_ptr<osgVerse::Pipeline> _pipeline;

protected:
    virtual osg::GraphicsOperation* createRenderer(osg::Camera* camera)
    {
        if (_pipeline.valid()) return _pipeline->createRenderer(camera);
        else return osgViewer::Viewer::createRenderer(camera);
    }
};

class OsgFramebufferObjectRenderer : public QQuickFramebufferObject::Renderer
{
public:
    OsgFramebufferObjectRenderer(osgViewer::Viewer* v) : _viewer(v) {}

    virtual void render()
    {
        QOpenGLContext::currentContext()->functions()->glUseProgram(0);
        if (_viewer.valid() && !_viewer->done()) _viewer->frame();
    }

    virtual QOpenGLFramebufferObject *createFramebufferObject(const QSize &size)
    {
        QOpenGLFramebufferObjectFormat format; format.setSamples(4);
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

        QOpenGLFramebufferObject* fbo = new QOpenGLFramebufferObject(size, format);
        osgViewer::GraphicsWindow* gw = static_cast<osgViewer::GraphicsWindow*>(
            _viewer->getCamera()->getGraphicsContext());
        if (gw) gw->setDefaultFboId(fbo->handle()); return fbo;
    }

protected:
    osg::observer_ptr<osgViewer::Viewer> _viewer;
};

class OsgFramebufferObject : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    OsgFramebufferObject(QQuickItem* parent = NULL);

    virtual QQuickFramebufferObject::Renderer* createRenderer() const
    { return new OsgFramebufferObjectRenderer(_viewer.get()); }

    virtual void initializeScene();
    osgViewer::Viewer* getViewer() { return _viewer.get(); }

protected:
    virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void hoverMoveEvent(QHoverEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _graphicsWindow;
    osg::ref_ptr<osgViewer::Viewer> _viewer;
    osgVerse::StandardPipelineParameters _params;
    QTimer _updateTimer;
    int _lastModifiers;
};

#endif
