#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="${FAKEPANIC_ROOT:-$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)}"
MONITOR="${FAKEPANIC_MONITOR:-FP-HEADLESS-$$}"
LAUNCHER="${FAKEPANIC_LAUNCHER:?}"
TARGET="${FAKEPANIC_TARGET:?}"
CAPTURE_W="${FAKEPANIC_CAPTURE_WIDTH:-1280}"
CAPTURE_H="${FAKEPANIC_CAPTURE_HEIGHT:-720}"
[[ "$MONITOR" =~ ^[A-Za-z0-9_-]+$ ]] || exit 2
[[ "$CAPTURE_W" =~ ^[0-9]{3,4}$ && "$CAPTURE_H" =~ ^[0-9]{3,4}$ ]] || exit 2
(( 10#$CAPTURE_W >= 320 && 10#$CAPTURE_W <= 3840 && 10#$CAPTURE_H >= 240 && 10#$CAPTURE_H <= 2160 )) || exit 2
for cmd in hyprctl jq wf-recorder ffmpeg gamescope python3; do
  command -v "$cmd" >/dev/null || { echo "Missing dependency / Нет зависимости: $cmd" >&2; exit 1; }
done
[[ -n "${HYPRLAND_INSTANCE_SIGNATURE:-}" ]] || { echo 'Hyprland session required / Требуется сеанс Hyprland' >&2; exit 1; }
case "$LAUNCHER" in
  lutris) command -v lutris >/dev/null; LAUNCH_CMD=(lutris "lutris:rungameid/$TARGET") ;;
  prism) command -v prismlauncher >/dev/null; LAUNCH_CMD=(prismlauncher -l "$TARGET") ;;
  *) exit 2 ;;
esac
BIN="${FAKEPANIC_BIN:-$ROOT/build/fakepanic-renderer}"
[[ -x "$BIN" ]] || { echo "Missing renderer / Нет рендерера: $BIN" >&2; exit 1; }
umask 077
LOG_DIR="${XDG_STATE_HOME:-$HOME/.local/state}/fakepanic"
mkdir -p "$LOG_DIR"
RUN_DIR="$(mktemp -d "${XDG_RUNTIME_DIR:-/tmp}/fakepanic.XXXXXXXX")"
FIFO="$RUN_DIR/capture.nut"
created=0; recorder_pid=''; gamescope_pid=''
cleanup() {
  trap - EXIT INT TERM
  for pid in "$recorder_pid" "$gamescope_pid"; do
    if [[ -n "$pid" ]]; then
      kill "$pid" 2>/dev/null || true
      for _ in {1..20}; do kill -0 "$pid" 2>/dev/null || break; sleep 0.05; done
      kill -KILL "$pid" 2>/dev/null || true
      wait "$pid" 2>/dev/null || true
    fi
  done
  rm -rf -- "$RUN_DIR"
  if (( created )); then hyprctl output remove "$MONITOR" >/dev/null 2>&1 || true; fi
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
monitor_exists() { hyprctl monitors -j | jq -e --arg name "$MONITOR" '.[] | select(.name == $name)' >/dev/null; }
if monitor_exists; then
  echo "Output already exists / Монитор уже существует: $MONITOR" >&2; exit 1
fi
hyprctl output create headless "$MONITOR" >/dev/null
created=1
for _ in {1..30}; do monitor_exists && break; sleep 0.2; done
monitor_exists || { echo 'Headless output unavailable / Не создан виртуальный монитор' >&2; exit 1; }
hyprctl eval "hl.monitor({output = \"$MONITOR\", mode = \"${CAPTURE_W}x${CAPTURE_H}@60\", position = \"auto\", scale = 1})" >"$LOG_DIR/monitor.log" 2>&1
for _ in {1..30}; do
  size="$(hyprctl monitors -j | jq -r --arg name "$MONITOR" '.[] | select(.name == $name) | "\(.width)x\(.height)"')"
  [[ "$size" == "${CAPTURE_W}x${CAPTURE_H}" ]] && break
  sleep 0.1
done
[[ "$size" == "${CAPTURE_W}x${CAPTURE_H}" ]] || { echo 'Output resize failed / Ошибка размера монитора' >&2; exit 1; }
WORKSPACE="$(hyprctl monitors -j | jq -r --arg name "$MONITOR" '.[] | select(.name == $name) | .activeWorkspace.id')"
[[ "$WORKSPACE" =~ ^[1-9][0-9]*$ ]] || { echo 'No workspace / Нет рабочего стола' >&2; exit 1; }
gamescope -w 1280 -h 720 -W "$CAPTURE_W" -H "$CAPTURE_H" -f -- "${LAUNCH_CMD[@]}" >"$LOG_DIR/gamescope.log" 2>&1 &
gamescope_pid=$!
ADDRESS=''
for _ in {1..180}; do
  ADDRESS="$(hyprctl clients -j | jq -r --argjson pid "$gamescope_pid" '[.[] | select(.class == "gamescope" and .pid == $pid)][0].address // empty')"
  [[ -n "$ADDRESS" ]] && break
  kill -0 "$gamescope_pid" 2>/dev/null || break
  sleep 0.3
done
[[ "$ADDRESS" =~ ^0x[0-9a-fA-F]+$ ]] || { echo "Gamescope failed / Ошибка Gamescope: $LOG_DIR/gamescope.log" >&2; exit 1; }
hyprctl eval "hl.dispatch(hl.dsp.window.move({workspace = $WORKSPACE, follow = false, window = \"address:$ADDRESS\"}))"
for _ in {1..30}; do
  game_monitor="$(hyprctl clients -j | jq -r --arg address "$ADDRESS" '.[] | select(.address == $address) | .monitor')"
  target_monitor="$(hyprctl monitors -j | jq -r --arg name "$MONITOR" '.[] | select(.name == $name) | .id')"
  [[ -n "$game_monitor" && "$game_monitor" == "$target_monitor" ]] && break
  sleep 0.2
done
[[ "$game_monitor" == "$target_monitor" ]] || { echo 'Window move failed / Не удалось переместить окно' >&2; exit 1; }
mkfifo "$FIFO"
wf-recorder -y -o "$MONITOR" -f "$FIFO" -c rawvideo -m nut >"$LOG_DIR/recorder.log" 2>&1 &
recorder_pid=$!
export FAKEPANIC_STREAM="$FIFO" FAKEPANIC_INPUT_HELPER="$ROOT/scripts/input-bridge.py"
export FAKEPANIC_GAMESCOPE_PID="$gamescope_pid"
echo "Esc: exit / выход. Logs / Логи: $LOG_DIR"
"$BIN" --game-stream "$@" 2> >(tee "$LOG_DIR/fakepanic.log" >&2)
