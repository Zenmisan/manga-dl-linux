#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace mangadl {

class SystemController : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qint64 totalDiskBytes READ totalDiskBytes NOTIFY storageChanged)
    Q_PROPERTY(qint64 freeDiskBytes READ freeDiskBytes NOTIFY storageChanged)
    Q_PROPERTY(qint64 usedDiskBytes READ usedDiskBytes NOTIFY storageChanged)
    Q_PROPERTY(double usedDiskFraction READ usedDiskFraction NOTIFY storageChanged)
    Q_PROPERTY(QString totalDiskFormatted READ totalDiskFormatted NOTIFY storageChanged)
    Q_PROPERTY(QString freeDiskFormatted READ freeDiskFormatted NOTIFY storageChanged)
    Q_PROPERTY(QString usedDiskFormatted READ usedDiskFormatted NOTIFY storageChanged)

    Q_PROPERTY(qint64 downloadDirBytes READ downloadDirBytes NOTIFY storageChanged)
    Q_PROPERTY(QString downloadDirFormatted READ downloadDirFormatted NOTIFY storageChanged)

    Q_PROPERTY(qint64 cacheDirBytes READ cacheDirBytes NOTIFY storageChanged)
    Q_PROPERTY(QString cacheDirFormatted READ cacheDirFormatted NOTIFY storageChanged)

    Q_PROPERTY(QString downloadLocation READ downloadLocation WRITE setDownloadLocation NOTIFY downloadLocationChanged)

public:
    explicit SystemController(QObject *parent = nullptr);
    ~SystemController() override = default;

    static SystemController &instance();

    qint64 totalDiskBytes() const { return m_totalDiskBytes; }
    qint64 freeDiskBytes() const { return m_freeDiskBytes; }
    qint64 usedDiskBytes() const { return m_usedDiskBytes; }
    double usedDiskFraction() const { return m_usedDiskFraction; }
    QString totalDiskFormatted() const { return m_totalDiskFormatted; }
    QString freeDiskFormatted() const { return m_freeDiskFormatted; }
    QString usedDiskFormatted() const { return m_usedDiskFormatted; }

    qint64 downloadDirBytes() const { return m_downloadDirBytes; }
    QString downloadDirFormatted() const { return m_downloadDirFormatted; }

    qint64 cacheDirBytes() const { return m_cacheDirBytes; }
    QString cacheDirFormatted() const { return m_cacheDirFormatted; }

    QString downloadLocation() const;
    void setDownloadLocation(const QString &path);

    Q_INVOKABLE void refreshStorage();
    Q_INVOKABLE void clearImageCache();
    Q_INVOKABLE static bool setAutostart(bool enabled);
    Q_INVOKABLE static bool isAutostartEnabled();

    static QString formatBytes(qint64 bytes);

signals:
    void storageChanged();
    void downloadLocationChanged();
    void cacheCleared();

private:
    static qint64 calculateDirectorySize(const QString &path);

    qint64 m_totalDiskBytes = 0;
    qint64 m_freeDiskBytes = 0;
    qint64 m_usedDiskBytes = 0;
    double m_usedDiskFraction = 0.0;
    QString m_totalDiskFormatted = QStringLiteral("0 GB");
    QString m_freeDiskFormatted = QStringLiteral("0 GB");
    QString m_usedDiskFormatted = QStringLiteral("0 GB");

    qint64 m_downloadDirBytes = 0;
    QString m_downloadDirFormatted = QStringLiteral("0 MB");

    qint64 m_cacheDirBytes = 0;
    QString m_cacheDirFormatted = QStringLiteral("0 MB");
};

} // namespace mangadl
