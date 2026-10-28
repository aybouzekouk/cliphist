#include "pastehelper.h"

#include <QGuiApplication>
#include <QTimer>
#include <QDebug>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

PasteHelper::PasteHelper(QObject *parent)
    : QObject(parent)
{
    if (auto *x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>())
        m_display = x11->display();
}

void PasteHelper::rememberActiveWindow()
{
    if (!m_display)
        return;

    Window focus = 0;
    int revert = 0;
    XGetInputFocus(m_display, &focus, &revert);

    if (focus == None || focus == PointerRoot) {
        m_target = 0;
        m_isTerminal = false;
        return;
    }
    m_target = focus;
    m_isTerminal = isTerminal(focus);
    qDebug() << "Cliphist: paste target" << focus << "isTerminal=" << m_isTerminal;
}

bool PasteHelper::isTerminal(unsigned long win) const
{
    // Walk up from the focused window to the nearest ancestor with a WM_CLASS
    // (the app's top-level window), then check that class for a terminal.
    Window w = static_cast<Window>(win);
    for (int depth = 0; w != None && depth < 16; ++depth) {
        XClassHint hint = {};
        if (XGetClassHint(m_display, w, &hint)) {
            const QString cls = QString::fromLocal8Bit(hint.res_class ? hint.res_class : "").toLower();
            const QString inst = QString::fromLocal8Bit(hint.res_name ? hint.res_name : "").toLower();
            if (hint.res_class) XFree(hint.res_class);
            if (hint.res_name) XFree(hint.res_name);

            static const char *kws[] = {
                "terminal", "konsole", "kitty", "alacritty", "xterm", "rxvt",
                "urxvt", "tilix", "terminator", "wezterm", "foot", "st-", "eterm"
            };
            for (const char *k : kws) {
                if (cls.contains(k) || inst.contains(k))
                    return true;
            }
            return false;
        }

        Window root = 0, parent = 0;
        Window *children = nullptr;
        unsigned int n = 0;
        if (!XQueryTree(m_display, w, &root, &parent, &children, &n))
            break;
        if (children) XFree(children);
        if (parent == None || parent == w || parent == root)
            break;
        w = parent;
    }
    return false;
}

void PasteHelper::paste()
{
    if (!m_display || m_target == 0)
        return;

    XSetInputFocus(m_display, m_target, RevertToPointerRoot, CurrentTime);
    XFlush(m_display);

    // Small settle delay so the target app is ready for the synthetic keys.
    QTimer::singleShot(60, this, &PasteHelper::inject);
}

void PasteHelper::inject()
{
    if (!m_display)
        return;

    const KeyCode ctrl = XKeysymToKeycode(m_display, XK_Control_L);
    const KeyCode shift = XKeysymToKeycode(m_display, XK_Shift_L);
    const KeyCode v = XKeysymToKeycode(m_display, XK_V);

    if (m_isTerminal) {
        // Terminals paste with Ctrl+Shift+V (plain Ctrl+V is "verbatim insert").
        XTestFakeKeyEvent(m_display, ctrl, True, CurrentTime);
        XTestFakeKeyEvent(m_display, shift, True, CurrentTime);
        XTestFakeKeyEvent(m_display, v, True, CurrentTime);
        XTestFakeKeyEvent(m_display, v, False, CurrentTime);
        XTestFakeKeyEvent(m_display, shift, False, CurrentTime);
        XTestFakeKeyEvent(m_display, ctrl, False, CurrentTime);
    } else {
        XTestFakeKeyEvent(m_display, ctrl, True, CurrentTime);
        XTestFakeKeyEvent(m_display, v, True, CurrentTime);
        XTestFakeKeyEvent(m_display, v, False, CurrentTime);
        XTestFakeKeyEvent(m_display, ctrl, False, CurrentTime);
    }
    XFlush(m_display);
}
