#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${1:-$ROOT/build/fakepanic-renderer}"
RUN_DIR="$(mktemp -d)"
trap 'rm -rf -- "$RUN_DIR"' EXIT
mkdir "$RUN_DIR/bin"
ffmpeg -v error -f lavfi -i testsrc2=size=1280x720:rate=24 -t 15 -c:v mpeg4 "$RUN_DIR/test.mp4"
cat > "$RUN_DIR/bin/prismlauncher" <<'LAUNCHER'
#!/usr/bin/env bash
export SDL_VIDEODRIVER=x11
exec "$FAKEPANIC_SMOKE_BIN" --video "$FAKEPANIC_SMOKE_CLIP" --mute --windowed --duration 12
LAUNCHER
chmod +x "$RUN_DIR/bin/prismlauncher"
export FAKEPANIC_SMOKE_BIN="$BIN" FAKEPANIC_SMOKE_CLIP="$RUN_DIR/test.mp4"
export FAKEPANIC_ROOT="$ROOT" FAKEPANIC_BIN="$BIN" FAKEPANIC_LAUNCHER=prism FAKEPANIC_TARGET=synthetic
export PATH="$RUN_DIR/bin:$PATH" XDG_STATE_HOME="$RUN_DIR/state"
env -u HYPRLAND_INSTANCE_SIGNATURE -u WAYLAND_DISPLAY -u DISPLAY \
  SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$ROOT/scripts/start-game.sh" --duration 6 --mute --stats
frames="$(sed -n 's/^\[stats\] frames=\([0-9]*\)$/\1/p' "$RUN_DIR/state/fakepanic/fakepanic.log" | tail -1)"
[[ "$frames" =~ ^[0-9]+$ ]] && (( frames > 0 ))
if ! grep -q 'ready on' "$RUN_DIR/state/fakepanic/fakepanic.log"; then
  echo 'Nested input unavailable' >&2; exit 1
fi
printf 'Game capture passed: %s frames\n' "$frames"
