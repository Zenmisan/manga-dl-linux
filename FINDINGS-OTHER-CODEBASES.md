# Findings in the other codebases

Problems found in the **web app, Windows, Android and the written specs** while building the Linux app.
Nothing here has been fixed in those projects: this file is the to-do list for correcting them later.

How to read an entry
- **ID**: stable reference (W = Windows, A = Android, X = shared extension scripts / web backend, S = spec documents).
- **Severity**: High = users hit it / feature broken; Medium = degraded or silently wrong; Low = polish or risk.
- **Confidence**: High = seen directly in code or in the audit output; Medium = inferred from reading code;
  Low = suspicion that needs a check. **Nothing in this file was confirmed by running the other apps.**
- **Fix**: what the Linux app does instead, which is also the suggested correction.

Status: all entries are **open**. When one is fixed in its project, change `open` to `fixed (date)`.

---

## Windows (`manga-dl-windows`)

### W-1 · open · High · Confidence: Medium-High
**The DOM shim has no `remove()`, so ad-stripping before reading chapter text probably throws.**
`src/MangaDl.Core/Extensions/DomShim.cs` exposes only `querySelector(All)`, `closest`, `getAttribute`,
`hasAttribute`, `tagName`, `nodeName`, `textContent`, `innerHTML`, `outerHTML`, `href`, `src`, `parentElement`.
The extension audit (`.agents/tracks/linux-backend-wiring_20261009/r1-audit.txt`) shows `.remove` at 26 call sites,
mostly `querySelectorAll('script, style, .ads, ...').forEach(e => e.remove())` in novel sources. 17 novel sources call
`.remove()` (allnovel, asianovel, chrysanthemumgarden, freewebnovel, libread, lightnoveltranslations, lightnovelworld,
novelbuddy, novelfire, novelfull, novelphoenix, novelsonline, ranobes, readhive, readlightnovel, readnovelfull,
royalroad) and 23 of the 40 extensions implement `getChapterText`. A missing method throws inside the script;
`ExtensionBridge` catches it, logs to the console and returns `null`. Likely effect: novel chapter text comes back
empty for those 17 sources.
Also missing and used by extensions: `classList.contains` (asurascans), `setAttribute` (novelsonline, ranobes),
`childNodes[0].textContent` (ranobes), `nextElementSibling` (freewebnovel), `document.createElement`
(comixto, a canvas). The audit's 12 `.attributes` hits are JSON objects in mangadex.js, not DOM, so no
`attributes` property is needed.
**Fix:** implement the missing members (Linux has them all), then add a test that runs every extension's
`getChapterText` against a saved HTML fixture.

### W-2 · open · Medium · Confidence: Medium
**Selectors that AngleSharp rejects are swallowed, so those lookups silently return nothing.**
`DomShim` wraps every query in `try { ... } catch { return null / empty }`. Extensions use two non-standard forms:
leading-combinator selectors (`'> a'`, `'> div > div'`, `'> span'`, 7 uses: bestlightnovel x4, readnovelfull x3),
`:contains("Author")` (readhive, inside a comma list, so the whole list fails) and `:has(span)` (bato, which
browsers support but older engines do not). A browser throws on the first two, so on Windows they simply find
nothing: bestlightnovel is probably unusable there, since every search/detail row is read through `'> a'`.
**Fix:** treat a leading `>` as scoped to the element; support `:contains()`; log selector parse failures
instead of hiding them.

### W-3 · open · High · Confidence: Medium
**Comix.to pages cannot be displayed: nothing handles the `comixto://img?...` page URLs.**
`comixto.js` returns pages as `comixto://img?url=...&seed=...&algo=...&encSeed=...&encLen=...&encAlgo=...`.
Reading them needs a byte-XOR decode plus a 5x5 tile unscramble (the script does it with browser canvas
APIs, which a native app does not have). A filename search of `manga-dl-windows/src` finds no descrambler.
**Fix:** native descrambler (Linux builds one in Phase 1: XOR + tile permutation with the same LCG/xorshift
math as `comixto.js`).

### W-4 · open · High · Confidence: Medium
**Asura Scans scrambled pages point to a backend endpoint that Windows never resolves.**
`asurascans.js` returns `/manga/asura-descramble?url=...&tiles=...&tileCols=...&tileRows=...` for scrambled
pages. `HttpService.ResolveUrl` only rewrites `/manga/proxy/html` and `/manga/proxy/json`; any other
relative path becomes `BackendUrl + path`, and Windows has no backend. The Android project already has the
right idea (`TileDescramblerInterceptor.kt`, tiles encoded in the URL fragment).
**Fix:** descramble on-device from the tile list, as Android does.

### W-5 · open · High · Confidence: High
**Chapter dates and author names are dropped for essentially every source.**
`ExtensionBridge.ParseMangaDetail` reads chapter `date` and detail `author`/`artist`. The audit shows the
scripts emit `published_at` (40 of 40 files) and `authors` (40 of 40), and none emit `date` (0 of 40) or
`artist` (0 of 40); only 1 file has `author`. So every chapter's release date is an empty string and no
author is shown.
**Fix:** accept both spellings (Linux reads `published_at`/`date`, `authors[]`/`author`/`artist`).

### W-6 · open · Medium · Confidence: High
**"Latest" can never work: the bridge never calls `getLatest`.**
All 40 extensions define `getLatest`, but `ExtensionBridge` only implements search, detail, pages, chapter text
and popular.
**Fix:** add `GetLatestAsync`, and use it for the Browse "Latest" tab.

### W-7 · open · Medium · Confidence: Medium
**Async handling only works by accident, and failures are invisible to the user.**
`SearchAsync`/`GetMangaDetailAsync`/`GetPagesAsync` start an async function and then read a global result at
once. That works only because `apiFetch` returns an already-resolved promise and Jint drains microtasks at the
end of `Execute`. Errors are caught in the script, logged with `Console.WriteLine`, and returned as empty
lists/`null`, so a broken source looks like "no results". The HTTP call inside blocks the thread
(`GetAwaiter().GetResult()`) while holding a `SemaphoreSlim`.
**Fix:** a real async bridge, with errors surfaced to the UI (Linux does both).

### W-8 · open · Low · Confidence: High
**Global search only queries the first 6 sources in dictionary order, and a failed source is dropped.**
`ExtensionManager.GlobalSearchAsync` uses `_registry.Values.Take(maxSources)`, so which sources are searched
is arbitrary (novel and manga sources are mixed) and `catch` turns every error into an empty group. Also, `r.CoverUrl ?? meta.Color` falls back to the source's brand colour in the same slot that holds the cover.
**Fix:** search the user's enabled sources, show per-source errors, use a separate field for the fallback colour.

### W-9 · open · Low · Confidence: High
**`encodeURIComponent`/`decodeURIComponent` are overridden with .NET functions that behave differently.**
`Uri.EscapeDataString` percent-encodes `! ' ( ) *`, which JavaScript leaves alone, and
`Uri.UnescapeDataString` does not throw on bad input where JavaScript throws `URIError`. `btoa`/`atob` also
encode UTF-8 bytes instead of Latin-1 characters like a browser. Probably harmless for current sources but a
trap for new ones.
**Fix:** keep the engine's built-in `encodeURIComponent`; only override `btoa`/`atob` if a source needs it.

### W-10 · open · Medium · Confidence: Medium
**System settings page still has placeholder text and a fake status.**
`SystemSettingsPage.xaml` contains literal `[user]` and `[date]` placeholders, a hard-coded
"Version 1.0.0 · up to date", a static `http://localhost:4567 · running · Online` label with no server behind it,
and sync toggles and an auto-backup dropdown that have no handlers in the XAML. (Seen when reading the file;
check the code-behind before fixing.)
**Fix:** bind real values; remove or implement the server label.

### W-11 · open · Low · Confidence: High
**The tracking documents disagree about Windows.**
`.agents/tracks.md` reports different passing-test totals (72, 88, 105) in different entries, and the
Apple spec (`manga-dl-apple` doc) says Windows has a LAN server toggle on `GeneralSettingsPage` and
24 extension scripts. The file has no such toggle and the folder has 40 scripts.
**Fix:** reconcile the numbers and correct the claims.

### W-12 · open · Low · Confidence: Medium
**No retry, backoff or rate limiting in `HttpService`; proxied page fetches send no `Referer`.**
One attempt with a 30 s timeout. Several manga CDNs reject requests without a matching `Referer`.
**Fix:** bounded retry with backoff for idempotent GETs; per-host referer rules.

---

## Android (`manga-dl-android`)

Only the descrambler has been read so far. A full audit has not been done.

### A-1 · open · Low · Confidence: High
**No comix.to descrambler found.** `app/.../data/source/descramble/` holds `TileDescramblerInterceptor.kt`
(Asura tile format only). A filename search found nothing for comix.to, whose pages need XOR decode plus a
5x5 tile unscramble (see W-3). Check whether comix.to works on Android at all.

### A-2 · open · Low · Confidence: Medium
**Descrambler swallows errors and silently serves the scrambled image.** `intercept()` catches every
exception and returns the original bytes, so a bad tile list shows a garbled page with nothing logged.
**Fix:** log the failure (and optionally show a placeholder).

---

## Shared extension scripts and web backend (`manga-dl/backend/app/services/extensions`)

### X-1 · open · Medium · Confidence: High
**There is no written contract for what extensions return, and the scripts are inconsistent with it.**
Hosts disagree: the scripts use `published_at`, `authors`, `cover_url`; Windows reads `date`, `author`
(W-5). Chapter `number` is sometimes a number, sometimes a string.
**Fix:** write one schema (fields, types, optionality) next to the scripts and add a validator test.

### X-2 · open · Medium · Confidence: High
**Scripts depend on backend-only URLs and browser APIs.** `getPages` can return
`/manga/asura-descramble?...` and `comixto://img?...`; `comixto.js` uses `Blob`, `URL.createObjectURL`,
`Image`, `document.createElement('canvas')`, `FileReader` and `Uint8Array`; and every script fetches through
`/manga/proxy/html|json?url=...`. Native apps have to special-case all of this.
**Fix:** make the scripts engine-neutral: return plain page descriptors
(`{url, descramble: {...}}`) and keep pixel work in the host.

### X-3 · open · Low · Confidence: High
**Non-standard selectors.** `'> a'`, `'> div > div'`, `'> span'` are invalid in browsers; `:contains()` is
jQuery-only. They work only in cheerio-style engines (see W-2).
**Fix:** rewrite as `:scope > a` and filter by text in JS.

### X-4 · open · Low · Confidence: High
**`madara.template.js` `_browseByPage` returns nothing for page 1 when the sorted page is empty.**
For `p = 1`, `offset` is 0, so `batchIdx = (0 - 1) % totalBatches = -1` and `genrePage = 0`;
`genres.slice(-5, 0)` is empty and the function returns `[]`. The "fall through to genre fetch" path never
produces results.
**Fix:** clamp `offset` and `batchIdx` at 0.

### X-5 · open · Medium · Confidence: High
**`flamecomics.js` expects top-level JSON array from `_fetchJson('/api/series')` to be an array, but Windows/native bridge returns an object wrapper.**
`search()` does `Array.isArray(all) ? all : []`. In web, `apiFetch` does `res.json()`, returning the array directly. On Windows, `apiFetch` returns `{ status, text, html, url, json }`, where `Array.isArray(out)` is false, so `search()` returns `[]`.
**Fix:** Linux attaches `status/text/html/url/json` directly to the array so `Array.isArray()` returns true and array methods work.

### X-6 · open · High · Confidence: High
**`freewebnovel.js` and `libread.js` send `acode` with URL path prefix to `/api/chapterlist.php`.**
`novelId` parsed from href keeps the path prefix (e.g. `novel/dragon-genesis` or `libread/weakest-beast-tamer...`). `/api/chapterlist.php` expects just the pure slug and rejects with "Novel verification failed" or HTTP 500, causing `getMangaDetail` to return 0 chapters.
**Fix:** strip `novel/` and `libread/` from `novelId` when sending `acode`.

### X-7 · open · Medium · Confidence: High
**`webtoons.js` search selector `.card_item` no longer matches webtoons.com markup.**
webtoons.com updated its search markup to `class="link _card_item"` (leading underscore on `<a>`). The selector `.card_item` fails to match, so search returns 0 results.
**Fix:** update selector to `._card_item, .card_item`.

### X-8 · open · Medium · Confidence: High
**`mangadex.js` includes external chapters whose page fetches fail with 404.**
MangaDex includes external chapters (e.g. hosted on MangaPlus with `externalUrl`) in the feed. Calling `getPages` on these IDs fails at `/at-home/server/{id}` because MangaDex does not host images for external chapters.
**Fix:** filter out chapters with `item.attributes.externalUrl` or handle external chapter URLs.

### X-9 · open · High · Confidence: High
**Several third-party sites use Cloudflare Turnstile / managed challenges or have dead domains.**
- Cloudflare bot challenges (403/503): `mangakakalot.gg`, `scribblehub.com`, `ranobes.net`.
- Domain parked / redirected: `manganato.com` redirects to `spinzywheel.com`.
- Site shut down: `lightnovelworld.org` / `.com` shut down and merged into `chikari.moe`.
- DNS failure: `novelarrow.com` (`novelbin.js`).

---

## Windows (`manga-dl-windows`)

### W-13 · open · Medium · Confidence: High
**`apiFetch` wrapper does not support scripts expecting top-level array JSON.**
When an API returns `[...]`, Windows `apiFetch` sets `out.json = parsed` but does not spread array elements or return the array, breaking `flamecomics.js` search.

---

## Spec documents

### S-1 · open · Low · Confidence: High
**`Native App Spec` and the Apple spec contradict the code.** They say 24 built-in sources (the repo has 38
real sources plus 2 templates), that Windows uses ChakraCore (it uses Jint), that Android embeds QuickJS
(another section says Android uses native Kotlin scrapers), and that Windows has a LAN server (W-11).
**Fix:** update the specs from the code, not the other way round.
