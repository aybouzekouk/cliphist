#include "globalhotkey.h"

#include <QGuiApplication>
#include <QDebug>

#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

GlobalHotkey::GlobalHotkey(QObject *parent)
    : QObject(parent)
{
    if (auto *x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>())
        m_display = x11->display();
}

void GlobalHotkey::grabCombos()
{
    if (!m_display)
        return;

    // Grab every combination of the lock keys (CapsLock, NumLock, Mod5) so
    // the hotkey still fires when any of them is active.
    const unsigned int modifiers = ControlMask | ShiftMask;
    const Window root = DefaultRootWindow(m_display);
    for (unsigned int lockBits = 0; lockBits < 8; ++lockBits) {
        unsigned int combo = modifiers;
        if (lockBits & 1) combo |= LockMask;
        if (lockBits & 2) combo |= Mod2Mask;
        if (lockBits & 4) combo |= Mod5Mask;
        XGrabKey(m_display, m_keycode, combo, root, True, GrabModeAsync, GrabModeAsync);
    }
    XSync(m_display, False);
}

bool GlobalHotkey::registerShortcut()
{
    if (!m_display)
        return false;
    m_keycode = XKeysymToKeycode(m_display, XK_V);
    grabCombos();
    return true;
}

void GlobalHotkey::suspend()
{
    if (!m_display)
        return;
    XUngrabKey(m_display, m_keycode, AnyModifier, DefaultRootWindow(m_display));
    XSync(m_display, False);
}

void GlobalHotkey::resume()
{
    grabCombos();
}

bool GlobalHotkey::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);
    if (eventType != "xcb_generic_event_t")
        return false;

    auto *ev = static_cast<xcb_generic_event_t *>(message);
    const uint8_t type = ev->response_type & ~0x80;
    if (type != XCB_KEY_PRESS && type != XCB_KEY_RELEASE)
        return false;

    auto *ke = reinterpret_cast<xcb_key_press_event_t *>(ev);
    if (ke->detail != m_keycode)
        return false;

    if (type == XCB_KEY_PRESS) {
        if (m_pressed) // ignore auto-repeat while held
            return true;
        m_pressed = true;
        qDebug() << "Cliphist: hotkey fired (state=0x" << Qt::hex << ke->state << ")";
        emit activated();
    } else {
        m_pressed = false;
    }
    return true;
}
