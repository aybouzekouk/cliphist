#pragma once

#include <QObject>

// Xlib is included only in the .cpp file (see globalhotkey.h).
typedef struct _XDisplay Display;

class PasteHelper : public QObject {
    Q_OBJECT

public:
    explicit PasteHelper(QObject *parent = nullptr);

    void rememberActiveWindow();

public slots:
    void paste();

private slots:
    void inject();

private:
    bool isTerminal(unsigned long win) const;

    Display *m_display = nullptr;
    unsigned long m_target = 0; // X11 Window (XID)
    bool m_isTerminal = false;
};
