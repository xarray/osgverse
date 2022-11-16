#if WIN32
#   pragma execution_character_set("utf-8")
#endif
#include "qt_header.h"

OsgSceneWidget::OsgSceneWidget(QWidget* parent)
    : QOpenGLWidget(parent), _lastModifiers(0), _firstFrame(true)
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setSamples(4); setFormat(format);
    _graphicsWindow = new osgViewer::GraphicsWindowEmbedded(
        this->x(), this->y(), this->width(), this->height());

    this->setFocusPolicy(Qt::StrongFocus);
    this->setMinimumSize(320, 240);
    this->setMouseTracking(true);

    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(update()));
    timer->start(20);
}

OsgSceneWidget::~OsgSceneWidget()
{
}

void OsgSceneWidget::paintGL()
{
    if (_viewer.valid())
    {
        if (_firstFrame)
        {
            GLuint defaultFboId = this->defaultFramebufferObject();
            _graphicsWindow->setDefaultFboId(defaultFboId);  // must set for internal FBO
            _firstFrame = false;
        }
        _viewer->frame();
    }
}

void OsgSceneWidget::resizeGL(int width, int height)
{
    _graphicsWindow->getEventQueue()->windowResize(this->x(), this->y(), width, height);
    _graphicsWindow->resized(this->x(), this->y(), width, height);
}

void OsgSceneWidget::closeEvent(QCloseEvent *event)
{
    if (_viewer.valid()) _viewer->setDone(true);
}

void OsgSceneWidget::keyPressEvent(QKeyEvent* event)
{
    QString keyString = event->text();
    const char* keyData = keyString.toLocal8Bit().data();
    //
    if (event->modifiers() != Qt::NoModifier)
    {
        int modifiers = event->modifiers(); _lastModifiers = modifiers;
        if (modifiers & Qt::ShiftModifier)
            _graphicsWindow->getEventQueue()->keyPress(osgGA::GUIEventAdapter::KEY_Shift_L);
        if (modifiers & Qt::ControlModifier)
            _graphicsWindow->getEventQueue()->keyPress(osgGA::GUIEventAdapter::KEY_Control_L);
        if (modifiers & Qt::AltModifier)
            _graphicsWindow->getEventQueue()->keyPress(osgGA::GUIEventAdapter::KEY_Alt_L);
    }
    _graphicsWindow->getEventQueue()->keyPress(osgGA::GUIEventAdapter::KeySymbol(*keyData));
}

void OsgSceneWidget::keyReleaseEvent(QKeyEvent* event)
{
    QString keyString = event->text();
    const char* keyData = keyString.toLocal8Bit().data();
    //
    if (_lastModifiers != Qt::NoModifier)
    {
        int modifiers = _lastModifiers; _lastModifiers = 0;
        if (modifiers & Qt::ShiftModifier)
            _graphicsWindow->getEventQueue()->keyRelease(osgGA::GUIEventAdapter::KEY_Shift_L);
        if (modifiers & Qt::ControlModifier)
            _graphicsWindow->getEventQueue()->keyRelease(osgGA::GUIEventAdapter::KEY_Control_L);
        if (modifiers & Qt::AltModifier)
            _graphicsWindow->getEventQueue()->keyRelease(osgGA::GUIEventAdapter::KEY_Alt_L);
    }
    _graphicsWindow->getEventQueue()->keyRelease(osgGA::GUIEventAdapter::KeySymbol(*keyData));
}

void OsgSceneWidget::mouseMoveEvent(QMouseEvent* event)
{
    _graphicsWindow->getEventQueue()->mouseMotion(static_cast<float>(event->x()),
                                                  static_cast<float>(event->y()));
}

void OsgSceneWidget::mousePressEvent(QMouseEvent* event)
{
    float x = static_cast<float>(event->x()), y = static_cast<float>(event->y());
    switch (event->button())
    {
    case Qt::LeftButton: _graphicsWindow->getEventQueue()->mouseButtonPress(x, y, 1); break;
    case Qt::RightButton: _graphicsWindow->getEventQueue()->mouseButtonPress(x, y, 3); break;
    case Qt::MiddleButton: _graphicsWindow->getEventQueue()->mouseButtonPress(x, y, 2); break;
    }
}

void OsgSceneWidget::mouseReleaseEvent(QMouseEvent* event)
{
    float x = static_cast<float>(event->x()), y = static_cast<float>(event->y());
    switch (event->button())
    {
    case Qt::LeftButton: _graphicsWindow->getEventQueue()->mouseButtonRelease(x, y, 1); break;
    case Qt::RightButton: _graphicsWindow->getEventQueue()->mouseButtonRelease(x, y, 3); break;
    case Qt::MiddleButton: _graphicsWindow->getEventQueue()->mouseButtonRelease(x, y, 2); break;
    }
}

void OsgSceneWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    float x = static_cast<float>(event->x()), y = static_cast<float>(event->y());
    switch (event->button())
    {
    case Qt::LeftButton: _graphicsWindow->getEventQueue()->mouseDoubleButtonPress(x, y, 1); break;
    case Qt::RightButton: _graphicsWindow->getEventQueue()->mouseDoubleButtonPress(x, y, 3); break;
    case Qt::MiddleButton: _graphicsWindow->getEventQueue()->mouseDoubleButtonPress(x, y, 2); break;
    }
}

void OsgSceneWidget::wheelEvent(QWheelEvent* event)
{
    int delta = event->delta();
    osgGA::GUIEventAdapter::ScrollingMotion motion = (delta > 0) ? osgGA::GUIEventAdapter::SCROLL_UP
                                                                 : osgGA::GUIEventAdapter::SCROLL_DOWN;
    _graphicsWindow->getEventQueue()->mouseScroll(motion);
}

bool OsgSceneWidget::event(QEvent* event)
{
    bool handled = QOpenGLWidget::event(event);
    switch (event->type())
    {
    case QEvent::KeyPress: case QEvent::KeyRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::Wheel:
        this->update(); break;
    default: break;
    }
    return handled;
}
