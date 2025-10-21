#!/usr/bin/env bash
set -euo pipefail

# -----------------------------
# Project paths
# -----------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PIKA_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
cd "$PROJECT_ROOT"

# 计算并行编译核数（Linux/macOS 兼容）
detect_jobs() {
  if command -v nproc >/dev/null 2>&1; then
    nproc
  elif [[ "$OSTYPE" == "darwin"* ]]; then
    sysctl -n hw.logicalcpu
  else
    echo 4
  fi
}
JOBS="${JOBS:-$(detect_jobs)}"

# 彩色输出
info(){ echo -e "\033[1;34m[INFO]\033[0m $*"; }
ok(){ echo -e "\033[1;32m[OK]\033[0m $*"; }
err(){ echo -e "\033[1;31m[ERR]\033[0m $*"; }

# -----------------------------
# Step 1: 检查并构建 pikiwidb
# -----------------------------
info "➡️ Step 1: 检查并构建 pikiwidb"
PIKA_LIB_DIR="$PROJECT_ROOT/vendor/lib/pika"
PIKA_LIB_CHECK="$PIKA_LIB_DIR/libpstd.a"   # \检测条件
PIKIWIDB_DIR="$PIKA_ROOT"

if [[ -f "$PIKA_LIB_CHECK" ]]; then
  ok "已构建，无需重复构建（发现 $PIKA_LIB_CHECK）"
else
  info "🔧 正在构建 pikiwidb ..."
  pushd "$PIKIWIDB_DIR" >/dev/null
  ./build.sh || { echo "❌ pikiwidb 构建失败"; exit 1; }
  popd >/dev/null
  ok "pikiwidb 构建完成"
fi

# -----------------------------
# Step 2: 复制静态库
# -----------------------------
info "➡️ Step 2: 复制静态库文件到 vendor/lib/pika/"
mkdir -p "$PIKA_LIB_DIR"

# 复制 output 下编译出的静态库
if [[ -d "$PIKA_ROOT/output/src" ]]; then
  find "$PIKA_ROOT/output/src" -type f -name "lib*.a" -exec cp -u {} "$PIKA_LIB_DIR/" \;
fi

# 复制 deps/lib 中的静态库（ExternalProject 产物）
if [[ -d "$PIKA_ROOT/deps/lib" ]]; then
  find "$PIKA_ROOT/deps/lib" -type f -name "lib*.a" -exec cp -u {} "$PIKA_LIB_DIR/" \;
fi

# 复制 deps/lib 中的静态库（ExternalProject 产物）
if [[ -d "$PIKA_ROOT/deps/lib64" ]]; then
  find "$PIKA_ROOT/deps/lib64" -type f -name "lib*.a" -exec cp -u {} "$PIKA_LIB_DIR/" \;
fi


ok "所有静态库已复制到 $PIKA_LIB_DIR/"

# -----------------------------
# Step 3: 构建主项目（CMake）
# -----------------------------
info "➡️ Step 3: 构建主项目"

BUILD_DIR="$PROJECT_ROOT/build"
mkdir -p "$BUILD_DIR"
pushd "$BUILD_DIR" >/dev/null

# OpenSSL 静态库路径兼容（有些系统是 lib64，有些是 lib）
OPENSSL_ROOT="$PROJECT_ROOT/third/openssl/install"
OPENSSL_INC="$OPENSSL_ROOT/include"
if [[ -f "$OPENSSL_ROOT/lib64/libssl.a" ]]; then
  OPENSSL_SSL="$OPENSSL_ROOT/lib64/libssl.a"
  OPENSSL_CRYPTO="$OPENSSL_ROOT/lib64/libcrypto.a"
else
  OPENSSL_SSL="$OPENSSL_ROOT/lib/libssl.a"
  OPENSSL_CRYPTO="$OPENSSL_ROOT/lib/libcrypto.a"
fi

# RocksDB 静态库（按你之前的构建，默认在 build/librocksdb.a）
ROCKSDB_INCLUDE="$PROJECT_ROOT/third/rocksdb/include"
ROCKSDB_LIB="$PROJECT_ROOT/third/rocksdb/build/librocksdb.a"

# CMAKE_PREFIX_PATH：aws-sdk-cpp / aws-crt-cpp / openssl 的 install
CMAKE_PREFIX="$PROJECT_ROOT/third/aws-sdk-cpp/install;$PROJECT_ROOT/third/aws-crt-cpp/install;$OPENSSL_ROOT"

cmake .. \
  -DOPENSSL_ROOT_DIR="$OPENSSL_ROOT" \
  -DOPENSSL_USE_STATIC_LIBS=ON \
  -DOPENSSL_INCLUDE_DIR="$OPENSSL_INC" \
  -DOPENSSL_SSL_LIBRARY="$OPENSSL_SSL" \
  -DOPENSSL_CRYPTO_LIBRARY="$OPENSSL_CRYPTO" \
  -DROCKSDB_INCLUDE_DIR="$ROCKSDB_INCLUDE" \
  -DROCKSDB_LIBRARY="$ROCKSDB_LIB" \
  -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX"

make -j"$JOBS" || { err "主项目构建失败"; exit 1; }

popd >/dev/null
ok "✅ 构建完成"
