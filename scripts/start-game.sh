#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="${FAKEPANIC_ROOT:-$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)}"
source "$ROOT/scripts/messages.sh"
LAUNCHER="${FAKEPANIC_LAUNCHER:?}"
TARGET="${FAKEPANIC_TARGET:?}"
CAPTURE_W="${FAKEPANIC_CAPTURE_WIDTH:-1280}"
CAPTURE_H="${FAKEPANIC_CAPTURE_HEIGHT:-720}"
FPS=24
args=("$@")
for ((i=0; i<${#args[@]}; i++)); do
  if [[ "${args[i]}" == --fps ]]; then FPS="${args[i+1]:-}"; fi
done
[[ "$CAPTURE_W" =~ ^[0-9]{3,4}$ && "$CAPTURE_H" =~ ^[0-9]{3,4}$ && "$FPS" =~ ^[0-9]{1,3}$ ]] || exit 2
(( 10#$CAPTURE_W >= 320 && 10#$CAPTURE_W <= 3840 && 10#$CAPTURE_H >= 240 && 10#$CAPTURE_H <= 2160 && 10#$FPS >= 1 && 10#$FPS <= 120 )) || exit 2
for cmd in gamescope gst-launch-1.0 gst-inspect-1.0 pw-dump jq python3; do
  command -v "$cmd" >/dev/null || { say "Missing dependency: %s" "Нет зависимости: %s" "$cmd" >&2; exit 1; }
done
for plugin in pipewiresrc videoscale videoconvert fdsink; do
  gst-inspect-1.0 "$plugin" >/dev/null 2>&1 || { say "Missing GStreamer plugin: %s" "Нет плагина GStreamer: %s" "$plugin" >&2; exit 1; }
done
case "$LAUNCHER" in
  lutris) command -v lutris >/dev/null; LAUNCH_CMD=(lutris "lutris:rungameid/$TARGET") ;;
  prism) command -v prismlauncher >/dev/null; LAUNCH_CMD=(prismlauncher -l "$TARGET") ;;
  *) exit 2 ;;
esac
BIN="${FAKEPANIC_BIN:-$ROOT/build/fakepanic-renderer}"
[[ -x "$BIN" ]] || { say "Missing renderer: %s" "Нет рендерера: %s" "$BIN" >&2; exit 1; }
umask 077
LOG_DIR="${XDG_STATE_HOME:-$HOME/.local/state}/fakepanic"
mkdir -p "$LOG_DIR"
gamescope_pid=''
cleanup() {
  trap - EXIT INT TERM
  if [[ -n "$gamescope_pid" ]]; then
    kill "$gamescope_pid" 2>/dev/null || true
    for _ in {1..30}; do kill -0 "$gamescope_pid" 2>/dev/null || break; sleep 0.05; done
    kill -KILL "$gamescope_pid" 2>/dev/null || true
    wait "$gamescope_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
# Force launched clients into Gamescope's nested X11 session on X11 and Wayland hosts.
SDL_VIDEODRIVER=x11 QT_QPA_PLATFORM=xcb GDK_BACKEND=x11 \
  gamescope --backend headless -w 1280 -h 720 -W "$CAPTURE_W" -H "$CAPTURE_H" -r "$FPS" \
  -- "${LAUNCH_CMD[@]}" >"$LOG_DIR/gamescope.log" 2>&1 &
gamescope_pid=$!
NODE=''
for _ in {1..100}; do
  NODE="$(pw-dump | jq -r --arg pid "$gamescope_pid" '
    [.[] | select(.type == "PipeWire:Interface:Client" and (.info.props["application.process.id"] | tostring) == $pid) | .id] as $clients |
    [.[] | select(.type == "PipeWire:Interface:Node" and .info.props["media.class"] == "Video/Source" and
      (.info.props["client.id"] as $id | $clients | index($id)))] |
    .[0].info.props["object.serial"] // empty')"
  [[ "$NODE" =~ ^[0-9]+$ ]] && break
  kill -0 "$gamescope_pid" 2>/dev/null || break
  sleep 0.2
done
[[ "$NODE" =~ ^[0-9]+$ ]] || { say "Gamescope/PipeWire failed: %s" "Ошибка Gamescope/PipeWire: %s" "$LOG_DIR/gamescope.log" >&2; exit 1; }
export FAKEPANIC_PIPEWIRE_NODE="$NODE" FAKEPANIC_INPUT_HELPER="$ROOT/scripts/input-bridge.py"
export FAKEPANIC_GAMESCOPE_PID="$gamescope_pid"
say "Esc: exit. Logs: %s" "Esc: выход. Логи: %s" "$LOG_DIR"
"$BIN" --game-stream "$@" 2> >(tee "$LOG_DIR/fakepanic.log" >&2)
