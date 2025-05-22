#if WIN32
#   pragma execution_character_set("utf-8")
#endif
#include "qt_header.h"
extern osgGA::GUIEventAdapter::KeySymbol getKey(int key, const QString& value);

QOpenGLFramebufferObject* OsgFramebufferObjectRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format; format.setSamples(4);
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

    QOpenGLFramebufferObject* fbo = new QOpenGLFramebufferObject(size, format);
    const OsgFramebufferObject* fboItem = qobject_cast<const OsgFramebufferObject*>(_fboParentItem);
    if (fboItem && fboItem->getGraphicsWindow())
        fboItem->getGraphicsWindow()->setDefaultFboId(fbo->handle());
    return fbo;
}

void OsgFramebufferObjectRenderer::render()
{
    const OsgFramebufferObject* fboItem = qobject_cast<const OsgFramebufferObject*>(_fboParentItem);
    QOpenGLContext::currentContext()->functions()->glUseProgram(0);
    QOpenGLContext::currentContext()->functions()->glDisable(GL_BLEND);
    QOpenGLContext::currentContext()->functions()->glDisable(GL_DEPTH_TEST);

    if (fboItem && fboItem->getViewer())
    {
        osgViewer::Viewer* viewer = fboItem->getViewer();
        if (!viewer->done()) viewer->frame();
#ifdef USE_QT6
        QQuickOpenGLUtils::resetOpenGLState();
#else
        fboItem->window()->resetOpenGLState();
#endif
    }
}

OsgFramebufferObject::OsgFramebufferObject(QQuickItem* parent)
:   QQuickFramebufferObject(parent), _lastModifiers(0)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setTextureFollowsItemSize(true);
    setMirrorVertically(true);

    _graphicsWindow = new osgViewer::GraphicsWindowEmbedded(0, 0, 640, 480);
    initializeScene();

    _updateTimer.setInterval(10); _updateTimer.start();
    connect(&_updateTimer, &QTimer::timeout, this, [this]() { update(); });
}

#ifdef USE_QT6
void OsgFramebufferObject::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
#else
void OsgFramebufferObject::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
#endif
{
    if (newGeometry.width() > 0 && newGeometry.height() > 0)
    {
        qreal retinaScale = 1.0f;
        if (window() != NULL) retinaScale = window()->devicePixelRatio();

        qreal w = newGeometry.width() * retinaScale, h = newGeometry.height() * retinaScale;
        _graphicsWindow->getEventQueue()->windowResize(newGeometry.x(), newGeometry.y(), w, h);
        _graphicsWindow->resized(newGeometry.x(), newGeometry.y(), w, h);
    }
#ifdef USE_QT6
    QQuickFramebufferObject::geometryChange(newGeometry, oldGeometry);
#else
    QQuickFramebufferObject::geometryChanged(newGeometry, oldGeometry);
#endif
    update();
}

void OsgFramebufferObject::keyPressEvent(QKeyEvent* event)
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

void OsgFramebufferObject::keyReleaseEvent(QKeyEvent* event)
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

void OsgFramebufferObject::mouseMoveEvent(QMouseEvent* event)
{
    _graphicsWindow->getEventQueue()->mouseMotion(static_cast<float>(event->x()),
                                                  static_cast<float>(event->y()));
}

void OsgFramebufferObject::hoverMoveEvent(QHoverEvent* event)
{
    QQuickItem::hoverMoveEvent(event);
}

void OsgFramebufferObject::mousePressEvent(QMouseEvent* event)
{
    float x = static_cast<float>(event->x()), y = static_cast<float>(event->y());
    switch (event->button())
    {
    case Qt::LeftButton: _graphicsWindow->getEventQueue()->mouseButtonPress(x, y, 1); break;
    case Qt::RightButton: _graphicsWindow->getEventQueue()->mouseButtonPress(x, y, 3); break;
    case Qt::MiddleButton: _graphicsWindow->getEventQueue()->mouseButtonPress(x, y, 2); break;
    }
}

void OsgFramebufferObject::mouseReleaseEvent(QMouseEvent* event)
{
    float x = static_cast<float>(event->x()), y = static_cast<float>(event->y());
    switch (event->button())
    {
    case Qt::LeftButton: _graphicsWindow->getEventQueue()->mouseButtonRelease(x, y, 1); break;
    case Qt::RightButton: _graphicsWindow->getEventQueue()->mouseButtonRelease(x, y, 3); break;
    case Qt::MiddleButton: _graphicsWindow->getEventQueue()->mouseButtonRelease(x, y, 2); break;
    }
}

void OsgFramebufferObject::mouseDoubleClickEvent(QMouseEvent *event)
{
    float x = static_cast<float>(event->x()), y = static_cast<float>(event->y());
    switch (event->button())
    {
    case Qt::LeftButton: _graphicsWindow->getEventQueue()->mouseDoubleButtonPress(x, y, 1); break;
    case Qt::RightButton: _graphicsWindow->getEventQueue()->mouseDoubleButtonPress(x, y, 3); break;
    case Qt::MiddleButton: _graphicsWindow->getEventQueue()->mouseDoubleButtonPress(x, y, 2); break;
    }
}

void OsgFramebufferObject::wheelEvent(QWheelEvent* event)
{
#ifdef USE_QT6
    int delta = event->pixelDelta().y();
#else
    int delta = event->delta();
#endif
    osgGA::GUIEventAdapter::ScrollingMotion motion = (delta > 0) ? osgGA::GUIEventAdapter::SCROLL_UP
                                                                 : osgGA::GUIEventAdapter::SCROLL_DOWN;
    _graphicsWindow->getEventQueue()->mouseScroll(motion);
}

osgGA::GUIEventAdapter::KeySymbol getKey(int key, const QString& value)
{
    if (!value.isEmpty())
    {
        char code = value[0].toLatin1();
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
