#!/bin/bash

set -eo pipefail

function run_as_root() {
  if [[ $(id -u) -eq 0 ]]; then
    "$@"
  else
    sudo "$@"
  fi
}

function install_deps() {
  echo "install deps before ..."
  if [[ $OS == *"macos"* ]]; then
    brew update
    brew install --overwrite python@3.12 autoconf protobuf llvm wget git
    brew install gcc@10 automake cmake make binutils
  elif [[ $OS == *"ubuntu"* ]]; then
    run_as_root apt-get update
    run_as_root apt-get install -y autoconf libprotobuf-dev protobuf-compiler clang-tidy
  elif [[ $OS == *"rocky"* ]]; then
    run_as_root dnf update -y
    run_as_root dnf install -y bash cmake
    run_as_root dnf install -y wget git autoconf gcc perl-Digest-SHA
    run_as_root dnf install -y tcl which tar g++ tar epel-release gcc-c++ libstdc++-devel
    run_as_root dnf install -y gcc-toolset-13
  else
    echo "not support $OS"
    return 1
  fi
  echo "install deps after success ..."
}

function configure_cmake() {
  echo "configure cmake before ..."
  if [[ $OS == *"macos"* ]]; then
    export CC=/usr/local/opt/gcc@10/bin/gcc-10
    cmake -B build -DCMAKE_C_COMPILER=/usr/local/opt/gcc@10/bin/gcc-10 -DUSE_PIKA_TOOLS=ON -DCMAKE_BUILD_TYPE=$BUILD_TYPE
  elif [[ $OS == *"ubuntu"* ]]; then
    cmake -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DUSE_PIKA_TOOLS=ON -DCMAKE_CXX_FLAGS="-s" -DCMAKE_EXE_LINKER_FLAGS="-s"
  elif [[ $OS == *"rocky"* ]]; then
    source /opt/rh/gcc-toolset-13/enable
    cmake -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DUSE_PIKA_TOOLS=ON -DCMAKE_CXX_FLAGS_DEBUG=-fsanitize=address
  fi
  echo "configure cmake after ..."
}

function build() {
  echo "build before ..."
  cmake --build build --config "$BUILD_TYPE"
  echo "build after success ..."
}

function install() {
  install_deps
  configure_cmake
  build
}

function checksum() {
  cd build/
  if [[ ! -f "$BINARY_NAME" || ! -s "$BINARY_NAME" ]]; then
    echo "Release binary is missing or empty: $BINARY_NAME" >&2
    return 1
  fi
  chmod +x "$BINARY_NAME"

  mkdir -p bin conf
  cp "$BINARY_NAME" "bin/$BINARY_NAME"
  cp ../conf/pika.conf conf/pika.conf

  tar -zcvf "$PACKAGE_NAME" "bin/$BINARY_NAME" conf/pika.conf

  shasum -a 256 "$PACKAGE_NAME" | cut -f1 -d' ' >"${PACKAGE_NAME}.sha256sum"
}

case $1 in
"install")
  OS=$2
  BUILD_TYPE=$3
  install
  ;;
"checksum")
  BINARY_NAME=$2
  PACKAGE_NAME=$3
  checksum
  ;;
*)
  echo "Invalid option"
  echo "option $1"
  exit 1
  ;;
esac
