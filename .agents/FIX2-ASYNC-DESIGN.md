# Fix #2: Async apiFetch - Design Document

## Current Problem

`HttpService::fetch()` blocks with `QEventLoop` (line 94-100 in HttpService.cpp):
```cpp
QEventLoop loop;
QTimer timer;
connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
loop.exec();  // ← BLOCKS here
```

While one `apiFetch` waits, Qt can deliver other `invokeScript` calls into the same JSContext, causing re-entrant nesting.

## Why It's Complex

1. **Static C function**: `jsApiFetch` is a C callback; can't easily store state
2. **One JSContext**: All extensions share one context; can't run truly parallel
3. **Promise resolution**: Need to keep resolve/reject functions alive across async boundary
4. **Event loop integration**: Need to pump JS jobs when network completes

## Solution Options

### Option A: Per-Call JSContext (COMPLEX, 8-10 hours)
- Create fresh JSContext for each `invokeScript` call
- Avoids re-entrancy entirely
- Memory overhead: ~1-2 MB per context
- Complexity: Medium-High

### Option B: Async HttpService with Callback Queue (MEDIUM, 3-4 hours)
- Make `HttpService::fetchAsync(url, callback)`
- Store Promise resolve/reject in a map keyed by request ID
- When QNetworkReply finishes, look up and call the resolver
- Requires passing JSContext* through the callback chain
- Complexity: Medium

### Option C: Accept Serial Execution, Fix Re-entrancy (EASY, 1 hour)
- Keep blocking fetch
- Add mutex to prevent re-entrant `invokeScript` calls
- Simple but searches stay serial
- Complexity: Low

## Recommended Approach (Option B)

```cpp
// HttpService.h
class HttpService : public QObject {
    struct PendingRequest {
        QNetworkReply *reply;
        JSContext *ctx;
        JSValue resolveFn;
        JSValue rejectFn;
    };
    
    void fetchAsync(const QString &url, const QVariantMap &opts,
                    JSContext *ctx, JSValue resolve, JSValue reject);
    
private slots:
    void onReplyFinished();
    
private:
    QHash<QNetworkReply*, PendingRequest> m_pending;
};

// JsWorker.cpp jsApiFetch
JSValue jsApiFetch(...) {
    JSValue resolveFns[2];
    JSValue promise = JS_NewPromiseCapability(ctx, resolveFns);
    
    // Store resolve/reject and initiate async fetch
    svc->fetchAsync(url, opts, ctx, 
                    JS_DupValue(ctx, resolveFns[0]),
                    JS_DupValue(ctx, resolveFns[1]));
    
    JS_FreeValue(ctx, resolveFns[0]);
    JS_FreeValue(ctx, resolveFns[1]);
    return promise;  // Returns immediately
}

// HttpService.cpp
void HttpService::fetchAsync(url, opts, ctx, resolve, reject) {
    QNetworkReply *reply = m_nam.get(request);
    m_pending[reply] = {reply, ctx, resolve, reject};
    connect(reply, &QNetworkReply::finished, this, &HttpService::onReplyFinished);
}

void HttpService::onReplyFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    PendingRequest req = m_pending.take(reply);
    
    // Build response object
    JSValue result = buildResponseObject(req.ctx, reply);
    
    // Resolve the Promise
    JSValue args[1] = {result};
    JS_Call(req.ctx, req.resolveFn, JS_UNDEFINED, 1, args);
    
    // Cleanup
    JS_FreeValue(req.ctx, req.resolveFn);
    JS_FreeValue(req.ctx, req.rejectFn);
    JS_FreeValue(req.ctx, result);
    reply->deleteLater();
    
    // CRITICAL: Pump JS jobs so Promise chain continues
    JSContext *pumpCtx;
    while (JS_ExecutePendingJob(JS_GetRuntime(req.ctx), &pumpCtx) > 0);
}
```

## Implementation Steps

1. Add `QHash<QNetworkReply*, PendingRequest>` to HttpService
2. Implement `fetchAsync` with signal connection
3. Modify `jsApiFetch` to pass JSContext and Promise functions
4. Add `onReplyFinished` slot that resolves Promise and pumps jobs
5. Test concurrent calls don't block each other

## Time Estimate
3-4 hours (medium complexity due to JSValue lifetime management)

## Status
Deferred - DOM fixes are higher priority for novel sources
