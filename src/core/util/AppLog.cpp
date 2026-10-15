#include "util/AppLog.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>

#include <cstdio>

namespace {

// The handler can be called from any thread, so every access to the file state goes through g_mutex.
// Nothing below may log (qDebug & co.) while holding the lock: the handler would deadlock on it.
QMutex g_mutex;
QFile g_file;
QtMessageHandler g_previous = nullptr;
bool g_installed = false;
qint64 g_maxBytes = AppLog::kDefaultMaxBytes;
int g_keep = AppLog::kDefaultKeep;

QString numbered(const QString &base, int n)
{
    return base + QLatin1Char('.') + QString::number(n);
}

bool openLocked()
{
    return g_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

// manga-dl.log -> manga-dl.log.1 -> manga-dl.log.2 ... (the oldest beyond g_keep is dropped)
void rotateLocked()
{
    const QString base = g_file.fileName();
    g_file.close();
    if (g_keep <= 1) {
        QFile::remove(base);
    } else {
        QFile::remove(numbered(base, g_keep - 1));
        for (int i = g_keep - 2; i >= 1; --i)
            QFile::rename(numbered(base, i), numbered(base, i + 1));
        QFile::rename(base, numbered(base, 1));
    }
    openLocked();
}

const char *levelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return "debug";
    case QtInfoMsg: return "info";
    case QtWarningMsg: return "warn";
    case QtCriticalMsg: return "error";
    case QtFatalMsg: return "fatal";
    }
    return "info";
}

void handler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const QString line = QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
        + QLatin1String(" [") + QLatin1String(levelName(type)) + QLatin1String("] ")
        + QString::fromUtf8(context.category ? context.category : "default")
        + QLatin1String(": ") + message + QLatin1Char('\n');
    const QByteArray bytes = line.toUtf8();

    QMutexLocker lock(&g_mutex);
    std::fwrite(bytes.constData(), 1, size_t(bytes.size()), stderr);
    if (g_file.isOpen()) {
        g_file.write(bytes);
        g_file.flush();
        if (g_file.size() > g_maxBytes)
            rotateLocked();
    }
}

} // namespace

namespace AppLog {

bool init(const QString &dir, qint64 maxBytes, int keep)
{
    QMutexLocker lock(&g_mutex);
    if (g_file.isOpen())
        g_file.close();
    g_maxBytes = maxBytes > 0 ? maxBytes : kDefaultMaxBytes;
    g_keep = keep > 0 ? keep : 1;

    QDir().mkpath(dir);
    g_file.setFileName(QDir(dir).filePath(QStringLiteral("manga-dl.log")));
    const bool opened = openLocked();

    if (!g_installed) {
        g_previous = qInstallMessageHandler(handler);
        g_installed = true;
    }
    return opened;
}

QString logFilePath()
{
    QMutexLocker lock(&g_mutex);
    return g_file.fileName();
}

void shutdown()
{
    QMutexLocker lock(&g_mutex);
    if (g_installed) {
        qInstallMessageHandler(g_previous);
        g_previous = nullptr;
        g_installed = false;
    }
    if (g_file.isOpen()) {
        g_file.flush();
        g_file.close();
    }
}

} // namespace AppLog
