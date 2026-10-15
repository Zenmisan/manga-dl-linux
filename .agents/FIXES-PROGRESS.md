# Blocker Fixes Progress - 2026-10-10

## ✅ COMPLETED FIXES

### Fix #1: Calling Convention ✅ DONE
- Changed ExtensionManager to send JSON arrays
- Modified JsWorker to spread arguments
- Added missing "comrademao" novel ID
- **Evidence**: Clean build, all tests pass

### Fix #5: decodeURIComponent ✅ DONE  
- Removed broken override using `.toLatin1()`
- Using QuickJS built-in (correct UTF-8 handling)
- **Evidence**: tst_jsworker::encodeURI passes

### Fix #10: Warnings ✅ DONE
- Fixed format string: `%lld` + cast for qsizetype
- Replaced deprecated `QVariant::type()` with `metaType().id()`
- **Evidence**: Zero warnings in build output

### Fix #7: Deadline Handling ✅ DONE
- Replaced shared deadline with QStack<qint64>
- pushDeadline() / popDeadline() for proper nesting
- checkTimeout() via worker pointer in interrupt handler
- **Evidence**: All tests pass, nested calls properly protected

### Fix #6: Descramble Bugs ✅ DONE
- Fixed seed parsing: `toUInt()` instead of `toInt()` (handles >2^31)
- Fixed URL decoding: removed double-decode, use `fromPercentEncoding()` correctly
- Added flat array support for Asura tiles: `[dst0, dst1, dst2, ...]`
- **Evidence**: tst_descramble 8/8 passed
- **Note**: WebP support requires `qt6-imageformats` package (not a code fix)

---

## 🔄 IN PROGRESS

### Fix #2: Async apiFetch (NEXT)
**Status**: Not started  
**Difficulty**: Medium (1-2 hours)  
**Plan**:
1. Make HttpService::fetch() return immediately
2. Emit signal when QNetworkReply::finished
3. Store per-callId state for Promise resolution
4. Test concurrent calls don't block

### Fix #3: DOM Live Accessors (HARD)
**Status**: Not started  
**Difficulty**: Hard (3-4 hours)  
**Plan**:
1. Replace snapshot strings with lazy JS getters
2. Use JS_DefinePropertyGetSet for innerHTML/textContent
3. Respect element.remove() in parent getters
4. Add missing APIs: parentElement, nextElementSibling, childNodes (with text), classList.contains

### Fix #4: Use-After-Free (depends on #3)
**Status**: Not started  
**Difficulty**: Easy (30 min after #3)  
**Plan**:
1. Elements hold strong JS reference to document
2. OR: Use shared_ptr<Document> everywhere

### Fix #8: Real Extension Tests (R3 Audit)
**Status**: Not started  
**Difficulty**: Medium (2-3 hours)  
**Plan**:
1. Save HTML fixtures for each of 40 extensions
2. Test search/getMangaDetail/getPages against fixtures
3. Document pass/fail in plan.md

### Fix #9: HttpService Gaps
**Status**: Partially complete  
**Completed**:
- ✅ Network errors surfaced (errorText set when !ok)

**Remaining**:
- Binary fetch with Referer header
- Charset detection (some sources use Shift-JIS/GB2312)

---

## 📊 Summary

**Completed**: 5/10 blockers  
**In Progress**: 0/10  
**Remaining**: 5/10  

**Quick wins done**: #5, #7, #10 (30 min total)  
**Medium fixes done**: #1, #6 (1.5 hours)  
**Hard fixes remaining**: #2, #3+#4, #8, #9 (6-8 hours estimated)

**Next**: Fix #2 (async apiFetch) to unblock parallel search
