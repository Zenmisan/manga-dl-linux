#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QVariant>
#include <QVariantMap>

#include <functional>
#include <memory>

/// Result of calling an extension method.
///
/// When ok is false, errorText starts with one of these tags (SourceError::fromText understands them):
///   [network]  no HTTP response (DNS, refused, TLS, reset, response too large)
///   [timeout]  the script ran too long, or never finished
///   [load]     the extension script did not evaluate or defines no `extension` object
///   [call]     bad arguments or the extension has no such method
/// Anything else is the text of an exception thrown by the script ("TypeError: ...").
struct JsResult {
    bool ok = false;
    QVariant value;      ///< decoded return value (QVariantMap, QVariantList, QString, number, bool, null)
    QString errorText;   ///< set when ok == false
};

/// One thread that owns a single JSRuntime and runs every extension script.
///
/// Design:
///  * One JSContext per extension id, created on first use and reused (the script is evaluated once, not per
///    call). Globals of one extension can never leak into another.
///  * Calls are cooperative. `apiFetch` returns a Promise and starts a non-blocking request; while a script awaits
///    the network the thread is free to serve other calls. There are no nested event loops, so calls never run
///    inside each other and one slow source cannot stall the rest.
///  * Every call has its own result slot (it is bound to the call id), so concurrent calls cannot mix results.
///  * Two limits guard against runaway scripts: a CPU "slice" (time the script may run between two awaits) and an
///    overall per-call timeout that includes network waiting.
///  * The runtime has a memory limit shared by all contexts.
class JsWorker : public QObject
{
    Q_OBJECT
public:
    struct Impl;  // implementation detail, defined in JsWorker.cpp

    /// Canned HTTP answer used by tests (see setFetchInterceptor).
    struct FakeResponse {
        int status = 200;
        QString text;
        QString url;
        QString transportError;  ///< non-empty: the request fails with this message (no HTTP response)
        int delayMs = 0;         ///< simulated latency
    };
    using FetchInterceptor =
        std::function<bool(const QString &url, const QVariantMap &opts, FakeResponse &out)>;

    /// One finished (or failed) request, as seen by the script. Used to record R3 fixtures.
    struct FetchTapEvent {
        QString requestUrl;   ///< argument the script passed to apiFetch (often a /manga/proxy/... URL)
        QVariantMap opts;
        int status = 0;
        QString text;
        QString finalUrl;
        QString transportError;  ///< non-empty when there was no HTTP response
    };
    using FetchTap = std::function<void(const FetchTapEvent &)>;

    static constexpr int kDefaultSliceMs = 10000;   ///< max CPU time between two awaits
    static constexpr int kDefaultCallMs = 90000;    ///< max wall time per call, network included
    static constexpr int kMemLimitMB = 256;         ///< whole runtime
    static constexpr qint64 kMaxBodyBytes = 40 * 1024 * 1024;  ///< larger responses are aborted

    explicit JsWorker(QObject *parent = nullptr);
    ~JsWorker() override;

    /// Creates the runtime. Runs on the worker thread; JsWorkerThread calls it before returning.
    Q_INVOKABLE void init();

    /// Calls `method` on the `extension` object of the script `code` (loaded once per `extensionId` and reused
    /// until the code changes). `argsJson` is a JSON ARRAY whose elements become the positional arguments:
    /// `["one piece", 2]` calls `extension.search("one piece", 2)`. The result arrives via scriptFinished().
    Q_INVOKABLE void invokeScript(quint64 callId, const QString &extensionId, const QString &code,
                                  const QString &method, const QString &argsJson);

    /// Overrides the limits (worker thread). Meant for tests.
    Q_INVOKABLE void setLimits(int sliceMs, int callMs);

    /// Replaces real networking with canned answers (worker thread). Meant for tests.
    /// Return false from the interceptor to fall through to a real request.
    void setFetchInterceptor(FetchInterceptor interceptor);

    /// Optional observer for every settled request (worker thread). Meant for fixture capture.
    void setFetchTap(FetchTap tap);

    /// Evaluates an expression in a scratch context (worker thread). Meant for tests.
    JsResult evalSync(const QString &code);

signals:
    void scriptFinished(quint64 callId, JsResult result);
    void initDone();

private:
    std::unique_ptr<Impl> d;
};

/// Starts a QThread, moves a JsWorker onto it and initialises it. When the constructor returns the worker is ready.
class JsWorkerThread : public QObject
{
    Q_OBJECT
public:
    explicit JsWorkerThread(QObject *parent = nullptr);
    ~JsWorkerThread() override;

    JsWorker *worker() const { return m_worker; }
    QThread *thread() const { return m_thread; }

private:
    QThread *m_thread = nullptr;
    JsWorker *m_worker = nullptr;
};
