#pragma once

#include "extensions/ExtensionManager.h"
#include "extensions/JsWorker.h"
#include "http/HttpService.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <QVariantMap>
#include <QVector>

/// On-disk pack of one extension's R3 run (search → detail → chapter) plus every apiFetch body.
struct R3Fetch {
    QString request;
    QString method = QStringLiteral("GET");
    QString body;
    int status = 0;
    QString text;
    QString finalUrl;
    QString transportError;
};

struct R3Step {
    bool ok = false;
    QString error;
    int count = 0;
    QString id;
    QString title;
};

struct R3Pack {
    QString id;
    QString query;
    QString recordedAt;
    R3Step search;
    R3Step detail;
    R3Step chapter;  ///< pages count in .count for manga; character count for novels
    bool novel = false;
    QVector<R3Fetch> fetches;
};

QString r3FetchKey(const QString &request, const QString &method, const QString &body);
QString r3CanonUrl(const QString &url);

bool r3SavePack(const R3Pack &pack, const QString &dir, QString *error = nullptr);
bool r3LoadPack(const QString &dir, R3Pack *pack, QString *error = nullptr);

JsWorker::FetchInterceptor r3ReplayInterceptor(const R3Pack &pack);

/// Drives search → detail → first chapter against the registered manager. Tap/interceptor must already be set.
void r3RunPipeline(ExtensionManager *mgr, const QString &extId, bool novel, const QString &query, R3Pack *pack);

bool r3Wait(const std::function<bool()> &cond, int msec);
