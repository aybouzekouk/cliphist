#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QString>
#include <QVector>

class QMimeData;
class QClipboard;
class QImage;

struct ClipEntry {
    QString kind;        // "text" | "image"
    QString text;        // full plain text (text entries)
    QString previewText; // single-line display text
    QString imagePath;   // absolute path to thumbnail png (image entries)
    QByteArray imageHash; // sha256 of image bytes, for dedup
    QByteArray mimeData;  // serialized QMimeData, for faithful paste
    qint64 timestamp = 0;
    int id = 0;
};

class ClipboardManager : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        KindRole = Qt::UserRole + 1,
        TextRole,
        ImagePathRole,
        TimeRole
    };

    explicit ClipboardManager(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void choose(int index);     // restore + paste the entry at index
    Q_INVOKABLE void clearHistory();

public slots:
    void setPaused(bool paused);

signals:
    void hideRequested();  // popup should animate out
    void doPaste();        // paste into the previously focused app

private slots:
    void onClipboardChanged();

private:
    void addFromMime(const QMimeData *mime);
    void restoreToClipboard(int index);
    void persist();
    void loadPersisted();
    void prune();
    QByteArray serializeMime(const QMimeData *mime) const;
    QMimeData *deserializeMime(const QByteArray &data) const;
    QByteArray hashImage(const QImage &img) const;
    QString cacheDir() const;
    static QString makePreview(const QString &text);
    static QString formatTime(qint64 timestamp);

    QVector<ClipEntry> m_entries;
    QClipboard *m_clipboard = nullptr;
    int m_nextId = 1;
    int m_cap = 50;
    bool m_paused = false;
    bool m_selfUpdate = false;
};
