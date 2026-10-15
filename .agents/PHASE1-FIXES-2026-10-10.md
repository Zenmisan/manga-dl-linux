# Phase 1 Critical Fixes — 2026-10-10

## Context
The 2026-10-10 review found 10 critical bugs blocking Phase 1 completion. This document tracks the systematic fixes applied.

## ✅ FIXED: Blocker #1 — Wrong Calling Convention

**Problem**: Extensions expect positional arguments `search(query, page)`, `getMangaDetail(id)`, `getPages(chapterId)`, but `invokeScript` was passing a single JSON object like `{"query":"...","page":1}`.

**Root Cause**: 
- `ExtensionManager` methods built JSON objects instead of JSON arrays
- `JsWorker::invokeScript` called the method with `JS_Call(..., 1, {argVal})` (one argument)
- Real extension scripts all use positional parameters

**Fix Applied**:
1. **ExtensionManager.cpp**: Changed all method wrappers to build `QJsonArray` instead of `QJsonObject`:
   - `search(extId, query, cb)` now sends `[query, 1]`
   - `getMangaDetail(extId, url, cb)` sends `[url]` 
   - `getPages(extId, chapterUrl, cb)` sends `[chapterUrl]`
   - `getChapterText(extId, chapterUrl, cb)` sends `[chapterUrl]`
   - `getPopular(extId, page, cb)` sends `[page]`
   - `getLatest(extId, page, cb)` sends `[page]`

2. **JsWorker.cpp**: Modified `invokeScript` to spread JSON array elements as separate positional arguments:
   ```cpp
   // Before: JS_Call(ctx, fn, extObj, 1, {argVal})
   // After:  JS_Call(ctx, fn, extObj, argCount, argValues.data())
   ```
   - Parse argJson as array, extract each element into QVector<JSValue>
   - Call with argCount and spread values
   - Backward compatible: if argJson is not an array, wraps it as single argument

**Verification**:
- Clean build: `[100%] Built target manga-dl`
- All existing tests pass (tst_jsworker: 16 passed)
- Ready for R3 audit against real extension scripts

**Files Modified**:
- `src/core/extensions/ExtensionManager.cpp` (6 methods + `#include <QJsonArray>`)
- `src/core/extensions/JsWorker.cpp` (invokeScript argument handling + `#include <QVector>`)

---

## ✅ FIXED: Missing Novel Extension ID

**Problem**: Review found 23 scripts with `getChapterText`, but `typeFromId()` only listed 22 novel IDs. `comrademao` was missing.

**Fix**: Added `"comrademao"` to the `novelIds` set in `ExtensionManager.cpp::typeFromId()`.

**Files Modified**:
- `src/core/extensions/ExtensionManager.cpp`

---

## 🔲 REMAINING BLOCKERS (prioritized)

### Blocker #2 — Synchronous apiFetch + Shared Context
**Impact**: HIGH (all network calls block the worker thread; concurrent searches are serialized)

**Problem**:
- `HttpService::fetch()` uses nested `QEventLoop` → synchronous blocking call
- `apiFetch` returns an already-resolved Promise (lines 546-554 in JsWorker.cpp)
- ONE worker thread + ONE JSContext shared by all extensions
- Re-entrant calls nest LIFO and overwrite shared `__mangadl_result/__mangadl_error` globals (lines 803-862)

**Required Fix**:
1. Make `HttpService::fetch()` truly async (emit signal when QNetworkReply::finished)
2. Store per-callId state instead of global `__mangadl_result`
3. OR: one JSContext per active extension (more memory, simpler concurrency)

**Affected Code**:
- `src/core/http/HttpService.{h,cpp}` 
- `src/core/extensions/JsWorker.cpp` (apiFetch + invokeScript Promise handling)

---

### Blocker #3 — DOM Snapshots Instead of Live Accessors

**Impact**: CRITICAL (17 novel sources use `element.remove()` then read `innerHTML`; all fail)

**Problem**:
- `makeElemValue()` recursively builds snapshot strings for `textContent`, `innerHTML`, `outerHTML` at creation time
- `element.remove()` does NOT update the parent's cached innerHTML
- `setAttribute('src', x)` then `innerHTML` is stale
- `parentElement`, `nextElementSibling` always null
- `childNodes` has no text nodes
- `classList.contains(x)` does not exist (only a `_classes_placeholder` property)

**Required Fix**:
- Replace snapshot properties with lazy JS getters reading live `dom::Document*`
- Define shared element prototype with:
  ```js
  Object.defineProperty(elemProto, 'innerHTML', {
    get: function() { return domInnerHTML(this.__domPtr, this.__index); }
  });
  ```
- `remove()` marks node deleted in the document tree; getters respect deletion

**Affected Code**:
- `src/core/dom/Dom.{h,cpp}` (element wrapper construction, see `JsWorker.cpp` ~line 200-300)

---

### Blocker #4 — Use-After-Free Risk

**Problem**: Element wrappers hold raw `dom::Document*` but don't keep a strong reference to the document's JS wrapper. If `parse(html).querySelector('x')` discards the document wrapper, its finalizer deletes the tree while elements still point to it.

**Required Fix**:
- Elements must hold a strong JS reference to their owning document (`JS_DupValue` in finalizer data)
- OR: use `std::shared_ptr<dom::Document>` in both wrapper types

---

### Finding #5 — decodeURIComponent Breaks Non-Latin-1

**Impact**: MEDIUM (affects sources with non-ASCII URLs)

**Problem**: 
```cpp
// Current override goes through toLatin1(), so ñ→? á→?
QString decoded = QUrl::fromPercentEncoding(percent.toLatin1());
```
The engine's built-in `decodeURIComponent` is already correct (finding W-9).

**Fix**: Remove the override entirely, use QuickJS built-in.

**Affected Code**: `JsWorker.cpp` jsDecodeURIComponent + setupGlobals registration

---

### Finding #6 — Descramble Bugs

**Impact**: MEDIUM (comixto and asura image sources fail)

**Problems**:
1. Comixto seeds parsed with `toInt()` instead of `toUInt()` → seeds >2^31 become 0 ("not scrambled")
2. URL values percent-decoded twice (once in parseComixtoUrl, again in the caller)
3. WebP needs Qt imageformats plugin
4. Asura: `tiles` should be flat int array `[src→dst]`, but `reassembleAsura` only handles object/pair format

**Files**: `src/core/extensions/Descramble.{h,cpp}`, image provider loading

---

### Finding #7 — Deadline Handling Broken

**Problem**: One shared `qint64 *deadline`; each call overwrites it and zeroes it when done → nested calls disable outer call's timeout.

**Fix**: Per-callId deadline map, or stack-based deadline restore.

---

### Finding #8 — No Real Extension Tests

**Problem**: All tests use toy scripts written for the JSON-object calling convention. No test validates the 40 real extension scripts.

**Required**: R3 audit — record HTML fixtures for each script, test search/getMangaDetail/getPages against saved responses.

---

### Finding #9 — HttpService Missing Features

**Problems**:
- No binary fetch with Referer (image loading needs headers)
- Charset always UTF-8 (some sources return Shift-JIS / GB2312)
- Transport failures (status 0) not surfaced as errors

---

### Finding #10 — Warnings to Clean

**Minor**:
- Format string warning: `qInfo("[ExtensionManager] loaded %d extensions", m_extensions.size())` expects int, got qsizetype
- Deprecated `QVariant::type()` calls (use `metaType().id()` instead)

---

## Next Steps

1. **Fix Blocker #5** (remove decodeURIComponent override) — quick win, 5 minutes
2. **Fix Blocker #2** (async apiFetch) — requires HttpService rewrite, 1-2 hours
3. **Fix Blocker #3** (DOM live accessors) — complex, 2-3 hours
4. **Fix Blocker #4** (use-after-free) — depends on #3
5. Run **R3 audit** with real extension fixtures
6. Fix descramble bugs (#6)
7. Fix deadline handling (#7)
8. Add HttpService binary fetch + charset detection (#9)
9. Clean warnings (#10)

**Phase 1 can be ticked complete** after R3 passes with all 40 real scripts.
