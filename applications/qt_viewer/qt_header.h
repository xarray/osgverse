#ifndef MANA_APP_QTVIEWER_HPP
#define MANA_APP_QTVIEWER_HPP

#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtOpenGL/QtOpenGL>
#ifdef USE_QT6
#   include <QtOpenGLWidgets/QtOpenGLWidgets>
#endif

#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osgViewer/Viewer>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>

// Change the macro to switch between QMainWindow and QWidget
#define USE_QMAINWINDOW 1

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

class RenderingThread : public QThread
{
    Q_OBJECT
public:
    RenderingThread(QObject* parent)
        : QThread(parent), _isRunning(true) {}

    virtual void run()
    {
        while (_isRunning)
        {
            emit updateRequired();
            msleep(15);
        }
    }

public slots:
    void setDone() { _isRunning = false; quit(); }

signals:
    void updateRequired();

protected:
    bool _isRunning;
};

class OsgSceneWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    OsgSceneWidget(QWidget* parent = 0);
    ~OsgSceneWidget();

    osg::Group* initializeScene(int argc, char** argv, osg::Group* sharedScene = NULL);

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
    osg::ref_ptr<osgViewer::Viewer> _viewer;

    osgVerse::StandardPipelineParameters _params;
    int _lastModifiers;
    bool _firstFrame;
};

#if USE_QMAINWINDOW
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow() : QMainWindow() {}

public slots:
    void addNewView();
    void removeLastView();

signals:
    void updateRequired();

protected:
    QList<OsgSceneWidget*> _allocatedWidgets;
    osg::observer_ptr<osg::Group> _sceneRoot;
};
#endif

#endif
