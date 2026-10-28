#pragma once

#include <QObject>
#include <QColor>

// Design tokens, exposed to QML as the "Theme" singleton.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QColor surface READ surface CONSTANT)
    Q_PROPERTY(QColor surfaceTop READ surfaceTop CONSTANT)
    Q_PROPERTY(QColor border READ border CONSTANT)
    Q_PROPERTY(QColor hairline READ hairline CONSTANT)
    Q_PROPERTY(QColor textPrimary READ textPrimary CONSTANT)
    Q_PROPERTY(QColor textSecondary READ textSecondary CONSTANT)
    Q_PROPERTY(QColor accent READ accent CONSTANT)
    Q_PROPERTY(QColor hover READ hover CONSTANT)
    Q_PROPERTY(QColor selection READ selection CONSTANT)
    Q_PROPERTY(qreal radius READ radius CONSTANT)
    Q_PROPERTY(qreal rowRadius READ rowRadius CONSTANT)
    Q_PROPERTY(qreal rowHeight READ rowHeight CONSTANT)
    Q_PROPERTY(qreal pad READ pad CONSTANT)
    Q_PROPERTY(int titleSize READ titleSize CONSTANT)
    Q_PROPERTY(int bodySize READ bodySize CONSTANT)
    Q_PROPERTY(int captionSize READ captionSize CONSTANT)

public:
    explicit Theme(QObject *parent = nullptr);

    QColor surface() const { return QColor(QStringLiteral("#181b22")); }
    QColor surfaceTop() const { return QColor(QStringLiteral("#222733")); }
    QColor border() const { return QColor(QStringLiteral("#2effffff")); }
    QColor hairline() const { return QColor(QStringLiteral("#12ffffff")); }
    QColor textPrimary() const { return QColor(QStringLiteral("#e9ecf2")); }
    QColor textSecondary() const { return QColor(QStringLiteral("#9aa3b2")); }
    QColor accent() const { return QColor(QStringLiteral("#8ab4ff")); }
    QColor hover() const { return QColor(QStringLiteral("#16ffffff")); }
    QColor selection() const { return QColor(QStringLiteral("#338ab4ff")); }
    qreal radius() const { return 16.0; }
    qreal rowRadius() const { return 10.0; }
    qreal rowHeight() const { return 46.0; }
    qreal pad() const { return 10.0; }
    int titleSize() const { return 13; }
    int bodySize() const { return 13; }
    int captionSize() const { return 11; }
};
