// Live R3 capture: one search → detail → chapter per built-in source, responses saved as fixtures.
//
//   r3-capture [--only id] [--query solo] [--out path]
//
// Network is required. Re-run whenever a source's markup changes.

#include "R3Support.h"

#include "extensions/ExtensionManager.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <algorithm>
#include <cstdio>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("r3-capture"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Record live extension fixtures for the R3 audit"));
    parser.addHelpOption();
    QCommandLineOption onlyOpt(QStringLiteral("only"), QStringLiteral("Capture a single extension id"),
                               QStringLiteral("id"));
    QCommandLineOption queryOpt(QStringLiteral("query"), QStringLiteral("Search query (default: solo)"),
                                QStringLiteral("q"), QStringLiteral("solo"));
    QCommandLineOption outOpt(QStringLiteral("out"), QStringLiteral("Fixture root directory"), QStringLiteral("dir"));
    parser.addOption(onlyOpt);
    parser.addOption(queryOpt);
    parser.addOption(outOpt);
    parser.process(app);

    QString outRoot = parser.value(outOpt);
    if (outRoot.isEmpty())
        outRoot = QStringLiteral(MANGADL_FIXTURES_DIR);

    ExtensionManager mgr;
    const int added = mgr.addScriptsFrom(QStringLiteral(MANGADL_ASSETS_DIR "/extensions"), true);
    if (added <= 0) {
        fprintf(stderr, "no scripts in assets/extensions\n");
        return 1;
    }

    QList<ExtensionInfo> all = mgr.extensions();
    const QString only = parser.value(onlyOpt);
    if (!only.isEmpty()) {
        all.erase(std::remove_if(all.begin(), all.end(),
                                 [&](const ExtensionInfo &i) { return i.id != only; }),
                  all.end());
        if (all.isEmpty()) {
            fprintf(stderr, "unknown extension: %s\n", qPrintable(only));
            return 1;
        }
    }

    const QString query = parser.value(queryOpt);
    QStringList report;
    int pass = 0;
    int fail = 0;

    QMutex tapMutex;
    for (const ExtensionInfo &info : all) {
        R3Pack pack;
        QVector<R3Fetch> liveFetches;
        JsWorker *w = mgr.worker();
        QMetaObject::invokeMethod(w, [w, &liveFetches, &tapMutex] {
            w->setFetchTap([&](const JsWorker::FetchTapEvent &ev) {
                QMutexLocker lock(&tapMutex);
                R3Fetch f;
                f.request = ev.requestUrl;
                f.method = ev.opts.value(QStringLiteral("method"), QStringLiteral("GET")).toString();
                f.body = ev.opts.value(QStringLiteral("body")).toString();
                f.status = ev.status;
                f.text = ev.text;
                f.finalUrl = ev.finalUrl;
                f.transportError = ev.transportError;
                liveFetches.append(f);
            });
        }, Qt::BlockingQueuedConnection);

        fprintf(stderr, "== %s (%s) query=%s\n", qPrintable(info.id),
                info.type == ExtensionType::Novel ? "novel" : "manga", qPrintable(query));
        r3RunPipeline(&mgr, info.id, info.type == ExtensionType::Novel, query, &pack);
        {
            QMutexLocker lock(&tapMutex);
            pack.fetches = liveFetches;
        }

        QString saveErr;
        const QString dir = QDir(outRoot).filePath(info.id);
        if (!r3SavePack(pack, dir, &saveErr))
            fprintf(stderr, "  save failed: %s\n", qPrintable(saveErr));

        const bool ok = pack.search.ok && pack.detail.ok && pack.chapter.ok;
        QString line = info.id;
        line += ok ? QStringLiteral(" PASS") : QStringLiteral(" FAIL");
        line += QStringLiteral(" search=") + (pack.search.ok ? QString::number(pack.search.count) : pack.search.error);
        line += QStringLiteral(" detail=") + (pack.detail.ok ? pack.detail.title.left(40) : pack.detail.error);
        line += QStringLiteral(" chapter=")
            + (pack.chapter.ok ? QString::number(pack.chapter.count) : pack.chapter.error);
        report.append(line);
        fprintf(stderr, "  %s  fetches=%d\n", qPrintable(line), int(pack.fetches.size()));
        ok ? ++pass : ++fail;

        QMetaObject::invokeMethod(w, [w] { w->setFetchTap({}); }, Qt::BlockingQueuedConnection);
    }

    QDir().mkpath(outRoot);
    QFile rf(QDir(outRoot).filePath(QStringLiteral("REPORT.md")));
    if (rf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&rf);
        ts << "# R3 live capture\n\n";
        ts << "Query: `" << query << "`  \n";
        ts << "Pass: " << pass << "  Fail: " << fail << "  Total: " << (pass + fail) << "\n\n";
        ts << "| Source | Result |\n|---|---|\n";
        for (const QString &line : report)
            ts << "| `" << line << "` |\n";
    }

    fprintf(stderr, "\n%d passed, %d failed (fixtures in %s)\n", pass, fail, qPrintable(outRoot));
    return fail == 0 ? 0 : 2;
}
