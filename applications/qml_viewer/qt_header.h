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
    OsgFramebufferObjectRenderer(const QQuickFramebufferObject* fbo) : _fboParentItem(fbo) {}
    virtual QOpenGLFramebufferObject* createFramebufferObject(const QSize &size);
    virtual void render();

protected:
    const QQuickFramebufferObject* _fboParentItem;
};

class OsgFramebufferObject : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    OsgFramebufferObject(QQuickItem* parent = NULL);

    virtual QQuickFramebufferObject::Renderer* createRenderer() const
    { return new OsgFramebufferObjectRenderer(this); }

    virtual void initializeScene();
    osgViewer::GraphicsWindow* getGraphicsWindow() const { return _graphicsWindow.get(); }
    osgViewer::Viewer* getViewer() const { return _viewer.get(); }

protected:
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
    osg::ref_ptr<osgViewer::Viewer> _viewer;
    osgVerse::StandardPipelineParameters _params;
    QTimer _updateTimer;
    int _lastModifiers;
};

#endif
