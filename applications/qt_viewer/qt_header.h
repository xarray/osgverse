#ifndef MANA_APP_QTVIEWER_HPP
#define MANA_APP_QTVIEWER_HPP

#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtOpenGL/QtOpenGL>

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

class OsgSceneWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    OsgSceneWidget(QWidget* parent = 0);
    ~OsgSceneWidget();

    void initializeScene(int argc, char** argv);

protected:
	virtual void paintGL();
    virtual void resizeGL(int width, int height);
    virtual void closeEvent(QCloseEvent *event);

    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual bool event(QEvent* event);
    
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _graphicsWindow;
    osg::ref_ptr<MyViewer> _viewer;
    int _lastModifiers;
    bool _firstFrame;
};

#endif
