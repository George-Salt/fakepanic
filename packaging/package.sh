#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${1:-$ROOT/build}"
OUT="$ROOT/dist"
VERSION="$("$BUILD/fakepanic-renderer" --version | awk '{print $2}')"
NAME="fakepanic-$VERSION-linux-$(uname -m)"
STAGE="$(mktemp -d)"
trap 'rm -rf -- "$STAGE"' EXIT
mkdir -p "$OUT" "$STAGE/$NAME"
cmake --install "$BUILD" --prefix "$STAGE/$NAME"
install -m 755 "$ROOT/install.sh" "$STAGE/$NAME/install.sh"
install -m 644 "$ROOT/README.md" "$ROOT/README.ru.md" "$ROOT/LICENSE" "$STAGE/$NAME/"
tar -C "$STAGE" -czf "$OUT/$NAME.tar.gz" "$NAME"
(cd "$BUILD" && cpack -G DEB -B "$OUT")
(cd "$OUT" && sha256sum ./*.tar.gz ./*.deb > SHA256SUMS)
