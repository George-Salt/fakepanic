#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PREFIX="${FAKEPANIC_PREFIX:-$HOME/.local}"
if [[ -x "$ROOT/share/fakepanic/fakepanic-renderer" ]]; then
  install -d "$PREFIX/bin" "$PREFIX/share/fakepanic" "$PREFIX/share/licenses/fakepanic"
  cp -R "$ROOT/share/fakepanic/." "$PREFIX/share/fakepanic/"
  install -m 755 "$ROOT/bin/fakepanic" "$PREFIX/bin/fakepanic"
  install -m 644 "$ROOT/LICENSE" "$PREFIX/share/licenses/fakepanic/LICENSE"
else
  command -v cmake >/dev/null || { echo 'cmake is required / Требуется cmake' >&2; exit 1; }
  cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$ROOT/build" --parallel "${FAKEPANIC_JOBS:-2}"
  cmake --install "$ROOT/build" --prefix "$PREFIX"
fi
printf '\nInstalled / Установлено: %s/bin/fakepanic\n' "$PREFIX"
printf 'PATH: %s/bin\n' "$PREFIX"
