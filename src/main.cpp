#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QCursor>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QDebug>
#include <QTimer>

#include "clipboardmanager.h"
#include "globalhotkey.h"
#include "pastehelper.h"
#include "theme.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Cliphist"));
    app.setOrganizationName(QStringLiteral("Cliphist"));
    app.setQuitOnLastWindowClosed(false);

    ClipboardManager clipboard;
    GlobalHotkey hotkey;
    PasteHelper pasteHelper;
    Theme theme;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("clipboard"), &clipboard);
    engine.rootContext()->setContextProperty(QStringLiteral("Theme"), &theme);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("Cliphist"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty())
        return -1;

    QQuickWindow *popup = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!popup)
        return -1;

    app.installNativeEventFilter(&hotkey);

    if (!hotkey.registerShortcut())
        qWarning() << "Cliphist: failed to register global hotkey (Ctrl+Shift+V)";

    QObject::connect(&hotkey, &GlobalHotkey::activated, [&] {
        // Defer to the event loop: showing a window from inside the native
        // event filter (which runs during X event processing) can leave the
        // window without proper input routing.
        QTimer::singleShot(0, [&] {
            qInfo() << "Cliphist: hotkey activated, toggling popup";
            if (popup->isVisible()) {
                popup->hide();
                return;
            }

            // Capture the focused window before our popup takes focus.
            pasteHelper.rememberActiveWindow();

            // Position near the cursor, clamped to the current screen.
            QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
            if (!screen)
                screen = QGuiApplication::primaryScreen();
            const QRect avail = screen->availableGeometry();
            const QPoint c = QCursor::pos();
            const int w = popup->width();
            const int h = popup->height();

            int x = c.x() + 12;
            int y = c.y() + 16;
            if (x + w > avail.right() + 1)
                x = c.x() - w - 12;
            if (y + h > avail.bottom() + 1)
                y = c.y() - h - 12;
            x = qBound(avail.left() + 4, x, avail.right() + 1 - w);
            y = qBound(avail.top() + 4, y, avail.bottom() + 1 - h);

            popup->setPosition(x, y);
            popup->show();
            popup->requestActivate();
        });
    });

    QObject::connect(&clipboard, &ClipboardManager::doPaste, [&] {
        // Release our own Ctrl+Shift+V grab so the synthetic paste (which may
        // be Ctrl+Shift+V for terminals) reaches the target instead of us.
        hotkey.suspend();
        pasteHelper.paste();
        QTimer::singleShot(150, &app, [&hotkey] { hotkey.resume(); });
    });

    // System tray icon (Quit / Pause / Clear). Harmless when no tray is available.
    QSystemTrayIcon tray(QIcon::fromTheme(QStringLiteral("edit-paste")));
    QMenu menu;
    QAction *pauseAct = menu.addAction(QStringLiteral("Pause watching"));
    pauseAct->setCheckable(true);
    QObject::connect(pauseAct, &QAction::toggled, &clipboard, &ClipboardManager::setPaused);
    QAction *clearAct = menu.addAction(QStringLiteral("Clear history"));
    QObject::connect(clearAct, &QAction::triggered, &clipboard, &ClipboardManager::clearHistory);
    menu.addSeparator();
    QAction *quitAct = menu.addAction(QStringLiteral("Quit"));
    QObject::connect(quitAct, &QAction::triggered, &app, &QCoreApplication::quit);
    tray.setContextMenu(&menu);
    tray.setToolTip(QStringLiteral("Cliphist — clipboard history"));

    if (QSystemTrayIcon::isSystemTrayAvailable())
        tray.show();
    else
        qWarning() << "Cliphist: no system tray available (tray menu disabled)";

    return app.exec();
}
