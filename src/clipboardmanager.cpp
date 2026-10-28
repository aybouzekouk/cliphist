#include "clipboardmanager.h"

#include <QClipboard>
#include <QMimeData>
#include <QGuiApplication>
#include <QImage>
#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QTimer>
#include <QUrl>
#include <QDataStream>

ClipboardManager::ClipboardManager(QObject *parent)
    : QAbstractListModel(parent)
    , m_clipboard(QGuiApplication::clipboard())
{
    loadPersisted();
    connect(m_clipboard, &QClipboard::dataChanged, this, &ClipboardManager::onClipboardChanged);

    // Seed with whatever is already on the clipboard.
    if (const QMimeData *mime = m_clipboard->mimeData(QClipboard::Clipboard))
        addFromMime(mime);
}

int ClipboardManager::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant ClipboardManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const ClipEntry &e = m_entries.at(index.row());
    switch (role) {
    case KindRole:      return e.kind;
    case TextRole:      return e.kind == QLatin1String("image") ? QString() : e.previewText;
    case ImagePathRole: return e.imagePath.isEmpty() ? QString() : QUrl::fromLocalFile(e.imagePath).toString();
    case TimeRole:      return formatTime(e.timestamp);
    default:            return {};
    }
}

QHash<int, QByteArray> ClipboardManager::roleNames() const
{
    return {
        { KindRole, "kind" },
        { TextRole, "text" },
        { ImagePathRole, "imagePath" },
        { TimeRole, "time" },
    };
}

void ClipboardManager::choose(int index)
{
    if (index < 0 || index >= m_entries.size())
        return;

    restoreToClipboard(index);
    emit hideRequested();
    // Give the popup time to animate out before injecting the paste keystroke.
    QTimer::singleShot(140, this, [this] { emit doPaste(); });
}

void ClipboardManager::clearHistory()
{
    beginResetModel();
    for (const ClipEntry &e : m_entries) {
        QFile::remove(cacheDir() + QStringLiteral("/thumb_%1.png").arg(e.id));
        QFile::remove(cacheDir() + QStringLiteral("/mime_%1.bin").arg(e.id));
    }
    m_entries.clear();
    endResetModel();
    persist();
}

void ClipboardManager::setPaused(bool paused)
{
    m_paused = paused;
}

void ClipboardManager::onClipboardChanged()
{
    if (m_paused)
        return;

    if (m_selfUpdate) {
        m_selfUpdate = false; // this change was ours (a paste), don't re-add
        return;
    }

    if (const QMimeData *mime = m_clipboard->mimeData(QClipboard::Clipboard))
        addFromMime(mime);
}

void ClipboardManager::addFromMime(const QMimeData *mime)
{
    bool hasImage = mime->hasImage();
    QImage img;
    if (hasImage) {
        img = mime->imageData().value<QImage>();
        if (img.isNull())
            hasImage = false;
    }

    QString text;
    if (!hasImage) {
        if (mime->hasText()) {
            text = mime->text();
        } else if (mime->hasUrls()) {
            QStringList urls;
            const QList<QUrl> list = mime->urls();
            urls.reserve(list.size());
            for (const QUrl &u : list)
                urls << u.toString();
            text = urls.join(QLatin1Char(' '));
        }
    }

    if (!hasImage && text.isEmpty())
        return;

    ClipEntry e;
    e.id = m_nextId++;
    e.timestamp = QDateTime::currentMSecsSinceEpoch();

    if (hasImage) {
        e.kind = QStringLiteral("image");
        e.imageHash = hashImage(img);
        e.imagePath = cacheDir() + QStringLiteral("/thumb_%1.png").arg(e.id);
    } else {
        e.kind = QStringLiteral("text");
        e.text = text;
        e.previewText = makePreview(text);
    }

    // Dedup against the most recent entry.
    if (!m_entries.isEmpty()) {
        const ClipEntry &head = m_entries.first();
        if (hasImage && head.kind == QLatin1String("image")) {
            if (e.imageHash == head.imageHash)
                return;
        } else if (!hasImage && head.kind != QLatin1String("image")) {
            if (head.text == text)
                return;
        }
    }

    if (hasImage) {
        QImage thumb = img;
        if (thumb.width() > 512 || thumb.height() > 512)
            thumb = thumb.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        thumb.save(e.imagePath, "PNG");
    }

    e.mimeData = serializeMime(mime);
    QFile mf(cacheDir() + QStringLiteral("/mime_%1.bin").arg(e.id));
    if (mf.open(QIODevice::WriteOnly)) {
        mf.write(e.mimeData);
        mf.close();
    }

    beginInsertRows(QModelIndex(), 0, 0);
    m_entries.prepend(e);
    endInsertRows();

    prune();
    persist();
}

void ClipboardManager::restoreToClipboard(int index)
{
    QMimeData *mime = deserializeMime(m_entries.at(index).mimeData);
    if (!mime)
        return;

    m_selfUpdate = true;
    m_clipboard->setMimeData(mime, QClipboard::Clipboard);
}

void ClipboardManager::prune()
{
    while (m_entries.size() > m_cap) {
        const int last = m_entries.size() - 1;
        beginRemoveRows(QModelIndex(), last, last);
        ClipEntry removed = m_entries.takeAt(last);
        endRemoveRows();
        QFile::remove(cacheDir() + QStringLiteral("/thumb_%1.png").arg(removed.id));
        QFile::remove(cacheDir() + QStringLiteral("/mime_%1.bin").arg(removed.id));
    }
}

void ClipboardManager::persist()
{
    QJsonArray arr;
    for (const ClipEntry &e : m_entries) {
        QJsonObject o;
        o.insert(QStringLiteral("kind"), e.kind);
        o.insert(QStringLiteral("text"), e.text);
        o.insert(QStringLiteral("preview"), e.previewText);
        o.insert(QStringLiteral("time"), static_cast<double>(e.timestamp));
        o.insert(QStringLiteral("id"), e.id);
        o.insert(QStringLiteral("hash"), QString::fromLatin1(e.imageHash.toBase64()));
        arr.append(o);
    }

    QFile f(cacheDir() + QStringLiteral("/history.json"));
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(arr).toJson());
        f.close();
    }
}

void ClipboardManager::loadPersisted()
{
    QFile f(cacheDir() + QStringLiteral("/history.json"));
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    f.close();

    int maxId = 0;
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        ClipEntry e;
        e.kind = o.value(QStringLiteral("kind")).toString();
        e.text = o.value(QStringLiteral("text")).toString();
        e.previewText = o.value(QStringLiteral("preview")).toString();
        e.timestamp = static_cast<qint64>(o.value(QStringLiteral("time")).toDouble());
        e.id = o.value(QStringLiteral("id")).toInt();
        e.imageHash = QByteArray::fromBase64(o.value(QStringLiteral("hash")).toString().toLatin1());
        e.imagePath = (e.kind == QLatin1String("image"))
                ? cacheDir() + QStringLiteral("/thumb_%1.png").arg(e.id)
                : QString();

        QFile mf(cacheDir() + QStringLiteral("/mime_%1.bin").arg(e.id));
        if (mf.open(QIODevice::ReadOnly)) {
            e.mimeData = mf.readAll();
            mf.close();
            m_entries.append(e);
            if (e.id > maxId)
                maxId = e.id;
        }
    }
    m_nextId = maxId + 1;
}

QByteArray ClipboardManager::serializeMime(const QMimeData *mime) const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    const QStringList formats = mime->formats();
    stream << formats;
    for (const QString &format : formats)
        stream << mime->data(format);
    return data;
}

QMimeData *ClipboardManager::deserializeMime(const QByteArray &data) const
{
    if (data.isEmpty())
        return nullptr;

    QDataStream stream(data);
    QStringList formats;
    stream >> formats;
    if (formats.isEmpty())
        return nullptr;

    auto *mime = new QMimeData;
    for (const QString &format : formats) {
        QByteArray bytes;
        stream >> bytes;
        mime->setData(format, bytes);
    }
    return mime;
}

QByteArray ClipboardManager::hashImage(const QImage &img) const
{
    QByteArray png;
    QBuffer buf(&png);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    return QCryptographicHash::hash(png, QCryptographicHash::Sha256);
}

QString ClipboardManager::cacheDir() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
            + QStringLiteral("/.local/share/cliphist");
    QDir().mkpath(dir);
    return dir;
}

QString ClipboardManager::makePreview(const QString &text)
{
    QString s = text;
    s.replace(QLatin1Char('\n'), QLatin1Char(' '));
    s.replace(QLatin1Char('\r'), QLatin1Char(' '));
    s.replace(QLatin1Char('\t'), QLatin1Char(' '));
    s = s.simplified();
    if (s.size() > 300) {
        s.truncate(300);
        s += QChar(0x2026); // ellipsis
    }
    return s;
}

QString ClipboardManager::formatTime(qint64 timestamp)
{
    const qint64 diff = QDateTime::currentMSecsSinceEpoch() - timestamp;
    if (diff < 60000)
        return QStringLiteral("now");
    if (diff < 3600000)
        return QStringLiteral("%1m").arg(diff / 60000);
    if (diff < 86400000)
        return QStringLiteral("%1h").arg(diff / 3600000);
    return QDateTime::fromMSecsSinceEpoch(timestamp).toString(QStringLiteral("dd MMM"));
}
