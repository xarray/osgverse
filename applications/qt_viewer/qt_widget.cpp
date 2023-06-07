#if WIN32
#   pragma execution_character_set("utf-8")
#endif
#include "qt_header.h"
extern osgGA::GUIEventAdapter::KeySymbol getKey(int key, const QString& value);

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

    //QTimer* timer = new QTimer(this);
    //connect(timer, SIGNAL(timeout()), SLOT(update()));
    //timer->start(15);
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
    paintGL();  // paint once to avoid flicker while resizing the widget
}

void OsgSceneWidget::closeEvent(QCloseEvent *event)
{
    if (_viewer.valid()) _viewer->setDone(true);
}

void OsgSceneWidget::keyPressEvent(QKeyEvent* event)
{
    osgGA::GUIEventAdapter::KeySymbol key = getKey(event->key(), event->text());
    if ((int)key == 0) return;  // not recognized
    
    if (event->modifiers() != Qt::NoModifier)
    {
        int modifiers = event->modifiers(); _lastModifiers = modifiers;
        if (modifiers & Qt::ShiftModifier) _graphicsWindow->getEventQueue()->keyPress(
            osgGA::GUIEventAdapter::KEY_Shift_L, osgGA::GUIEventAdapter::KEY_Shift_L);
        if (modifiers & Qt::ControlModifier) _graphicsWindow->getEventQueue()->keyPress(
            osgGA::GUIEventAdapter::KEY_Control_L, osgGA::GUIEventAdapter::KEY_Control_L);
        if (modifiers & Qt::AltModifier) _graphicsWindow->getEventQueue()->keyPress(
            osgGA::GUIEventAdapter::KEY_Alt_L, osgGA::GUIEventAdapter::KEY_Alt_L);
    }
    _graphicsWindow->getEventQueue()->keyPress(key, event->key());
}

void OsgSceneWidget::keyReleaseEvent(QKeyEvent* event)
{
    osgGA::GUIEventAdapter::KeySymbol key = getKey(event->key(), event->text());
    if ((int)key == 0) return;  // not recognized
    
    if (_lastModifiers != Qt::NoModifier)
    {
        int modifiers = _lastModifiers; _lastModifiers = 0;
        if (modifiers & Qt::ShiftModifier) _graphicsWindow->getEventQueue()->keyRelease(
            osgGA::GUIEventAdapter::KEY_Shift_L, osgGA::GUIEventAdapter::KEY_Shift_L);
        if (modifiers & Qt::ControlModifier) _graphicsWindow->getEventQueue()->keyRelease(
            osgGA::GUIEventAdapter::KEY_Control_L, osgGA::GUIEventAdapter::KEY_Control_L);
        if (modifiers & Qt::AltModifier) _graphicsWindow->getEventQueue()->keyRelease(
            osgGA::GUIEventAdapter::KEY_Alt_L, osgGA::GUIEventAdapter::KEY_Alt_L);
    }
    _graphicsWindow->getEventQueue()->keyRelease(key, event->key());
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

osgGA::GUIEventAdapter::KeySymbol getKey(int key, const QString& value)
{
    char* keyData = value.toLocal8Bit().data();
    if (!value.isEmpty())
    {
        char code = *keyData;
        if ((code >= 'a' && code <= 'z') || (code >= 'A' && code <= 'Z') ||
            (code >= '0' && code <= '9'))
            return (osgGA::GUIEventAdapter::KeySymbol)code;
    }

    switch (key)
    {
    case Qt::Key_Escape: return osgGA::GUIEventAdapter::KEY_Escape;
    case Qt::Key_Tab: return osgGA::GUIEventAdapter::KEY_Tab;
    case Qt::Key_Backspace: return osgGA::GUIEventAdapter::KEY_BackSpace;
    case Qt::Key_Return: return osgGA::GUIEventAdapter::KEY_Return;
    case Qt::Key_Enter: return osgGA::GUIEventAdapter::KEY_Return;
    case Qt::Key_Insert: return osgGA::GUIEventAdapter::KEY_Insert;
    case Qt::Key_Delete: return osgGA::GUIEventAdapter::KEY_Delete;
    case Qt::Key_Pause: return osgGA::GUIEventAdapter::KEY_Pause;
    case Qt::Key_Print: return osgGA::GUIEventAdapter::KEY_Print;
    case Qt::Key_SysReq: return osgGA::GUIEventAdapter::KEY_Sys_Req;
    case Qt::Key_Clear: return osgGA::GUIEventAdapter::KEY_Clear;
    case Qt::Key_Home: return osgGA::GUIEventAdapter::KEY_Home;
    case Qt::Key_End: return osgGA::GUIEventAdapter::KEY_End;
    case Qt::Key_Left: return osgGA::GUIEventAdapter::KEY_Left;
    case Qt::Key_Up: return osgGA::GUIEventAdapter::KEY_Up;
    case Qt::Key_Right: return osgGA::GUIEventAdapter::KEY_Right;
    case Qt::Key_Down: return osgGA::GUIEventAdapter::KEY_Down;
    case Qt::Key_PageUp: return osgGA::GUIEventAdapter::KEY_Page_Up;
    case Qt::Key_PageDown: return osgGA::GUIEventAdapter::KEY_Page_Down;
    case Qt::Key_CapsLock: return osgGA::GUIEventAdapter::KEY_Caps_Lock;
    case Qt::Key_NumLock: return osgGA::GUIEventAdapter::KEY_Num_Lock;
    case Qt::Key_ScrollLock: return osgGA::GUIEventAdapter::KEY_Scroll_Lock;
    case Qt::Key_F1: return osgGA::GUIEventAdapter::KEY_F1;
    case Qt::Key_F2: return osgGA::GUIEventAdapter::KEY_F2;
    case Qt::Key_F3: return osgGA::GUIEventAdapter::KEY_F3;
    case Qt::Key_F4: return osgGA::GUIEventAdapter::KEY_F4;
    case Qt::Key_F5: return osgGA::GUIEventAdapter::KEY_F5;
    case Qt::Key_F6: return osgGA::GUIEventAdapter::KEY_F6;
    case Qt::Key_F7: return osgGA::GUIEventAdapter::KEY_F7;
    case Qt::Key_F8: return osgGA::GUIEventAdapter::KEY_F8;
    case Qt::Key_F9: return osgGA::GUIEventAdapter::KEY_F9;
    case Qt::Key_F10: return osgGA::GUIEventAdapter::KEY_F10;
    case Qt::Key_F11: return osgGA::GUIEventAdapter::KEY_F11;
    case Qt::Key_F12: return osgGA::GUIEventAdapter::KEY_F12;
    case Qt::Key_Space: return osgGA::GUIEventAdapter::KEY_Space;
    case Qt::Key_Exclam: return osgGA::GUIEventAdapter::KEY_Exclaim;
    case Qt::Key_QuoteDbl: return osgGA::GUIEventAdapter::KEY_Quotedbl;
    case Qt::Key_NumberSign: return osgGA::GUIEventAdapter::KEY_Hash;
    case Qt::Key_Dollar: return osgGA::GUIEventAdapter::KEY_Dollar;
    case Qt::Key_Percent: return (osgGA::GUIEventAdapter::KeySymbol)0x25;  // '%'
    case Qt::Key_Ampersand: return osgGA::GUIEventAdapter::KEY_Ampersand;
    case Qt::Key_Apostrophe: return osgGA::GUIEventAdapter::KEY_Quote;
    case Qt::Key_ParenLeft: return osgGA::GUIEventAdapter::KEY_Leftparen;
    case Qt::Key_ParenRight: return osgGA::GUIEventAdapter::KEY_Rightparen;
    case Qt::Key_Asterisk: return osgGA::GUIEventAdapter::KEY_Asterisk;
    case Qt::Key_Plus: return osgGA::GUIEventAdapter::KEY_Plus;
    case Qt::Key_Comma: return osgGA::GUIEventAdapter::KEY_Comma;
    case Qt::Key_Minus: return osgGA::GUIEventAdapter::KEY_Minus;
    case Qt::Key_Period: return osgGA::GUIEventAdapter::KEY_Period;
    case Qt::Key_Slash: return osgGA::GUIEventAdapter::KEY_Slash;
    case Qt::Key_Colon: return osgGA::GUIEventAdapter::KEY_Colon;
    case Qt::Key_Semicolon: return osgGA::GUIEventAdapter::KEY_Semicolon;
    case Qt::Key_Less: return osgGA::GUIEventAdapter::KEY_Less;
    case Qt::Key_Equal: return osgGA::GUIEventAdapter::KEY_Equals;
    case Qt::Key_Greater: return osgGA::GUIEventAdapter::KEY_Greater;
    case Qt::Key_Question: return osgGA::GUIEventAdapter::KEY_Question;
    case Qt::Key_At: return osgGA::GUIEventAdapter::KEY_At;
    case Qt::Key_BracketLeft: return osgGA::GUIEventAdapter::KEY_Leftbracket;
    case Qt::Key_Backslash: return osgGA::GUIEventAdapter::KEY_Backslash;
    case Qt::Key_BracketRight: return osgGA::GUIEventAdapter::KEY_Rightbracket;
    case Qt::Key_AsciiCircum: return osgGA::GUIEventAdapter::KEY_Caret;
    case Qt::Key_Underscore: return osgGA::GUIEventAdapter::KEY_Underscore;
    case Qt::Key_QuoteLeft: return osgGA::GUIEventAdapter::KEY_Backquote;
    default: break;
    }
    return (osgGA::GUIEventAdapter::KeySymbol)0;
}
