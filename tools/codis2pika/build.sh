#!/bin/bash

set -e

# go1.24 normalizes `go 1.23` -> `go 1.23.0` and injects a toolchain line during
# -mod=mod builds. CentOS7 CI rejects the three-segment form, so build with
# -mod=mod but restore go.mod/go.sum to the committed state on exit.
GO_MOD_FILE=$(cd "$(dirname "$0")" && pwd)/go.mod
GO_SUM_FILE=$(cd "$(dirname "$0")" && pwd)/go.sum
_c2p_mod_bak=/tmp/codis2pika.go.mod.bak
_c2p_sum_bak=/tmp/codis2pika.go.sum.bak
cp "$GO_MOD_FILE" "$_c2p_mod_bak"
cp "$GO_SUM_FILE" "$_c2p_sum_bak"
restore_go_files() {
  cp "$_c2p_mod_bak" "$GO_MOD_FILE"
  cp "$_c2p_sum_bak" "$GO_SUM_FILE"
}
trap restore_go_files EXIT
export GOFLAGS=-mod=mod

echo "[ BUILD RELEASE ]"
BIN_DIR=$(pwd)/bin/
rm -rf "$BIN_DIR"
mkdir -p "$BIN_DIR"

# build the current platform
echo "try build for current platform"
go build -v -trimpath  -gcflags '-N -l' -o "$BIN_DIR/codis2pika" "./cmd/codis2pika"
echo "build success"

for g in "linux" "darwin"; do
  for a in "amd64" "arm64"; do
    echo "try build GOOS=$g GOARCH=$a"
    export GOOS=$g
    export GOARCH=$a
    go build -v -trimpath  -gcflags '-N -l' -o "$BIN_DIR/codis2pika-$g-$a" "./cmd/codis2pika"
    unset GOOS
    unset GOARCH
    echo "build success"
  done
done

cp codis2pika.toml "$BIN_DIR"

if [ "$1" == "dist" ]; then
  echo "[ DIST ]"
  cd bin
  cp -r ../filters ./
  tar -czvf ./codis2pika.tar.gz ./codis2pika.toml  ./codis2pika-* ./filters
  rm -rf ./filters
  cd ..
fi
