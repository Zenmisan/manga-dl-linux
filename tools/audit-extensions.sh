#!/usr/bin/env bash
# Read-only census of what the extension scripts need from the host app (DOM surface, selectors, globals,
# apiFetch endpoints, result field names). The output decides how big the DOM/bridge layer has to be.
#
#   bash tools/audit-extensions.sh [extensions-dir] > some-file.txt
set -uo pipefail

app="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dir="${1:-$app/assets/extensions}"
cd "$dir" || { echo "directory not found: $dir" >&2; exit 1; }

# \x27 = ' , \x22 = " , \x60 = `   (the grep -P patterns below use these escapes so they need no shell quoting tricks)

section() { printf '\n==== %s ====\n' "$1"; }

section "files"
ls -1 *.js | wc -l

section "selector strings passed to querySelector / querySelectorAll / closest / matches (count, selector)"
grep -ohP '(querySelectorAll|querySelector|closest|matches)\(\s*([\x27\x22\x60])\K(?:(?!\2).)*(?=\2)' *.js \
    | sort | uniq -c | sort -rn

section "pseudo-classes / pseudo-elements appearing inside those selectors (count) - attribute values like og:title are noise"
grep -ohP '(querySelectorAll|querySelector|closest|matches)\(\s*([\x27\x22\x60])\K(?:(?!\2).)*(?=\2)' *.js \
    | grep -oP '::?[a-zA-Z-]+(\([^)]*\))?' | sort | uniq -c | sort -rn

section "selector attribute operators used ([a=b] [a^=b] [a$=b] [a*=b] [a~=b] [a|=b], plus case flag)"
grep -ohP '(querySelectorAll|querySelector|closest|matches)\(\s*([\x27\x22\x60])\K(?:(?!\2).)*(?=\2)' *.js \
    | grep -oP '\[[^\]=\s]+\s*([\^\$\*~\|]?=)|\s[iIsS]\]' | sed -E 's/\[[^]=^$*~|[:space:]]+//' | sort | uniq -c | sort -rn

section "DOM-ish members used (count). .href/.src are noisy: they also match plain objects."
grep -ohP '\.(remove|setAttribute|removeAttribute|hasAttribute|getAttribute|getAttributeNames|classList|className|childNodes|children|childElementCount|firstElementChild|lastElementChild|firstChild|lastChild|nextElementSibling|previousElementSibling|nextSibling|previousSibling|parentElement|parentNode|closest|matches|contains|cloneNode|appendChild|append|prepend|insertBefore|insertAdjacentHTML|replaceWith|replaceChild|removeChild|innerText|textContent|innerHTML|outerHTML|dataset|attributes|nodeType|nodeName|tagName|href|src|srcset|getElementById|getElementsByClassName|getElementsByTagName|documentElement|createElement|createTextNode|ownerDocument|isConnected|offsetWidth|naturalWidth|getBoundingClientRect|toString|normalize|trim)\b' *.js \
    | sort | uniq -c | sort -rn

section "host globals / browser-only APIs (count). Engine builtins like Math, JSON, Date are not listed."
grep -ohP '\b(setTimeout|setInterval|clearTimeout|clearInterval|queueMicrotask|URLSearchParams|URL|TextDecoder|TextEncoder|Buffer|crypto|window|document|navigator|localStorage|sessionStorage|FormData|Headers|Request|Response|AbortController|XMLHttpRequest|structuredClone|Intl|require|importScripts|process|performance|escape|unescape|decodeURI|encodeURI|DOMParser|XMLSerializer|fetch|Uint8Array|Uint8ClampedArray|ArrayBuffer|DataView|Blob|File|Image|OffscreenCanvas|ImageData|BigInt|Proxy|Reflect|WeakMap|WeakRef|Symbol|atob|btoa|console)\b' *.js \
    | sort | uniq -c | sort -rn

section "lines that touch document. or window. (file:line:text)"
grep -nP '\b(document|window|navigator)\.' *.js | cut -c1-200 | head -80

section "apiFetch first-argument literal prefixes before any '?' (count, prefix)"
grep -ohP 'apiFetch\(\s*[\x27\x22\x60]\K[^\x27\x22\x60?]*' *.js | sort | uniq -c | sort -rn

section "apiFetch calls whose first argument is NOT a string literal (count, first token)"
grep -ohP 'apiFetch\(\s*\K[^\x27\x22\x60\s)][^,)]{0,40}' *.js | sort | uniq -c | sort -rn | head -40

section "relative (backend) endpoints mentioned anywhere: strings starting with /manga/ /sources/ /api/ /proxy/ /image (count)"
grep -ohP '[\x27\x22\x60]/(manga|sources|api|proxy|image|extensions|novels|novel)[^\x27\x22\x60?]*' *.js | sort | uniq -c | sort -rn

section "apiFetch options: methods used"
grep -ohP 'method:\s*[\x27\x22]\w+[\x27\x22]' *.js | sort | uniq -c | sort -rn

section "request header names set by extensions (count)"
grep -ohP '[\x27\x22](Referer|Cookie|Origin|Authorization|Content-Type|Accept[A-Za-z-]*|X-[A-Za-z-]+|User-Agent|Sec-[A-Za-z-]+)[\x27\x22]\s*:' *.js | sort | uniq -c | sort -rn

section "request bodies: files that pass body:"
grep -lP '\bbody\s*:' *.js

section "methods defined directly on the extension object (count of files)"
awk '
  /(var|const|let)[ \t]+extension[ \t]*=[ \t]*\{/ { f=1; next }
  f && /^\};?[ \t]*$/ { f=0 }
  f && match($0, /^  (async[ \t]+)?[A-Za-z_]+[ \t]*\(/) {
      s = substr($0, RSTART, RLENGTH - 1)
      sub(/^  (async[ \t]+)?/, "", s); sub(/[ \t]+$/, "", s)
      print s
  }' *.js | sort | uniq -c | sort -rn

section "files NOT using the 'var/const/let extension = {' form (other export styles)"
grep -LP '(var|const|let)[ \t]+extension[ \t]*=' *.js

section "which files mention each optional extension method (names as text)"
for m in search getMangaDetail getPages getChapterText getPopular getLatest getGenres getFilters browse getChapters getHome; do
    printf '%-16s %s\n' "$m" "$(grep -lP "\b$m\b" *.js | wc -l)"
done

section "result field names: number of files with  name:  as an object key"
for k in id title cover_url cover url status provider description genres chapters author authors artist artists number date released published_at scanlator language type tags rating; do
    printf '%-14s %s\n' "$k" "$(grep -lP "\b$k\s*:" *.js | wc -l)"
done

section "script size (bytes), largest first"
wc -c *.js | sort -rn | head -12

section "EXACT LINES: rare DOM members (file:line:text)"
grep -nP '\.(attributes|setAttribute|removeAttribute|childNodes|nextElementSibling|previousElementSibling|parentElement|classList|contains|createElement|outerHTML|tagName|nodeType)\b' *.js | cut -c1-220 | head -80

section "EXACT LINES: rare globals (URL, fetch, XMLHttpRequest, require, Blob, Image, Uint8Array)"
grep -nP '\b(URL|fetch|XMLHttpRequest|require|Blob|Image|Uint8Array)\b' *.js | cut -c1-220 | head -60

section "EXACT LINES: .src / .href property reads (not getAttribute)"
grep -nP '\.(src|href)\b' *.js | cut -c1-220 | head -30

section "EXACT LINES: non-standard selectors (leading combinator, :contains, :has)"
grep -nP '(querySelectorAll|querySelector|closest|matches)\(\s*[\x27\x22\x60](\s*>|[^\x27\x22\x60]*:(contains|has)\()' *.js | cut -c1-240

section "EXACT LINES: .remove( calls (first 30)"
grep -nP '\.remove\(' *.js | cut -c1-200 | head -30
