#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "$ROOT/scripts/messages.sh" ]]; then
  source "$ROOT/scripts/messages.sh"
else
  # shellcheck source=scripts/messages.sh
  source "$ROOT/share/fakepanic/scripts/messages.sh"
fi
PREFIX="${FAKEPANIC_PREFIX:-$HOME/.local}"
if [[ -x "$ROOT/share/fakepanic/fakepanic-renderer" ]]; then
  install -d "$PREFIX/bin" "$PREFIX/share/fakepanic" "$PREFIX/share/licenses/fakepanic"
  cp -R "$ROOT/share/fakepanic/." "$PREFIX/share/fakepanic/"
  install -m 755 "$ROOT/bin/fakepanic" "$PREFIX/bin/fakepanic"
  install -m 644 "$ROOT/LICENSE" "$PREFIX/share/licenses/fakepanic/LICENSE"
else
  command -v cmake >/dev/null || { say 'cmake is required' 'Требуется cmake' >&2; exit 1; }
  cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$ROOT/build" --parallel "${FAKEPANIC_JOBS:-2}"
  cmake --install "$ROOT/build" --prefix "$PREFIX"
fi
printf '\n'
say 'Installed: %s/bin/fakepanic' 'Установлено: %s/bin/fakepanic' "$PREFIX"
printf 'PATH: %s/bin\n' "$PREFIX"
