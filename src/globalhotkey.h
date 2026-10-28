#pragma once

#include <QObject>
#include <QAbstractNativeEventFilter>

// Xlib is included only in the .cpp file. Its headers define the macros
// Bool/Status/True/False/None, which collide with Qt if pulled in here.
typedef struct _XDisplay Display;
typedef unsigned char KeyCode;

class GlobalHotkey : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    explicit GlobalHotkey(QObject *parent = nullptr);

    bool registerShortcut();
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

public slots:
    void suspend(); // temporarily release the grab (while injecting a paste)
    void resume();

signals:
    void activated();

private:
    void grabCombos();

    Display *m_display = nullptr;
    KeyCode m_keycode = 0;
    bool m_pressed = false;
};
