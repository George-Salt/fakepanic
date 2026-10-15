#!/usr/bin/env bash
FP_LANG=en
FP_ARGS=("$@")
for ((FP_I=0; FP_I<${#FP_ARGS[@]}; FP_I++)); do
  if [[ "${FP_ARGS[FP_I]}" == --lang ]]; then
    FP_LANG=en
    [[ "${FP_ARGS[FP_I+1]:-}" != ru ]] || FP_LANG=ru
  fi
done
export FAKEPANIC_LANG="$FP_LANG"
say() {
  local message="$1"
  [[ "$FP_LANG" != ru ]] || message="$2"
  shift 2
  local value
  for value in "$@"; do
    printf '%s%s' "${message%%\%s*}" "$value"
    message="${message#*%s}"
  done
  printf '%s\n' "$message"
}
