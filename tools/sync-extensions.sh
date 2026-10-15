#!/usr/bin/env bash
# Copies the shared extension scripts into assets/extensions/ and checks them against the Windows app's copies.
#
#   bash tools/sync-extensions.sh [web-backend-extensions-dir] [windows-extensions-dir]
#
# The copies in assets/extensions/ are generated: never hand-edit them. Fix the script upstream and re-run this.
set -euo pipefail

app="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
src="${1:-$app/../../manga-dl/backend/app/services/extensions}"
win="${2:-$app/../../manga-dl-windows/src/MangaDl.App/Assets/extensions}"
dst="$app/assets/extensions"

[ -d "$src" ] || { echo "source directory not found: $src" >&2; exit 1; }

mkdir -p "$dst"
# Drop stale copies first so a script deleted upstream disappears here too.
find "$dst" -maxdepth 1 -name '*.js' -delete
cp -- "$src"/*.js "$dst"/
(cd "$dst" && sha256sum -- *.js > MANIFEST.sha256)
echo "copied $(find "$dst" -maxdepth 1 -name '*.js' | wc -l) scripts from $src"

if [ -d "$win" ]; then
    differ=0
    for f in "$dst"/*.js; do
        b="$(basename "$f")"
        if [ ! -f "$win/$b" ]; then
            echo "only in the web backend: $b"; differ=1
        elif ! cmp -s "$f" "$win/$b"; then
            echo "DIFFERS from the Windows copy: $b"; differ=1
        fi
    done
    for f in "$win"/*.js; do
        [ -f "$dst/$(basename "$f")" ] || { echo "only in the Windows app: $(basename "$f")"; differ=1; }
    done
    if [ "$differ" -eq 0 ]; then
        echo "identical to the Windows copies"
    fi
else
    echo "(Windows copies not found at $win, skipped comparison)"
fi
