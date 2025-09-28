#!/usr/bin/env bash
set -euo pipefail

# ==========================
# Orchestrator
# ==========================
# 功能：
#  - 一键/分步编译 third 目录下的依赖：openssl、curl、aws-crt-cpp、aws-sdk-cpp、rocksdb、hiredis
#  - 生成 protobuf
#
# 使用：
#  1) 默认全量构建：
#       ./shell/pre_build.sh
#  2) 指定步骤（可多选）：
#       ./shell/pre_build.sh openssl curl aws-crt-cpp aws-sdk-cpp rocksdb hiredis proto
#  3) 可选参数：
#       INSTALL_ROOT=<绝对或相对路径>   # 依赖安装根目录，默认：$PROJECT_ROOT/third
#       JOBS=<并行数>                   # 默认自动检测
#
# 示例：
#   INSTALL_ROOT=/data/ospp/pika_batch_ingest/third JOBS=16 ./shell/pre_build.sh curl aws-sdk-cpp
#
# 说明：
#  - 尽量使用相对 PROJECT_ROOT 的路径，避免硬编码绝对路径
#  - 已做幂等处理：存在则 update，不存在则 add/init
#  - macOS 和 Linux 均可；RocksDB 会根据系统使用不同编译选项
#
# ==========================

# ---------- 基本环境 ----------
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# 可通过环境变量覆盖
INSTALL_ROOT="${INSTALL_ROOT:-$PROJECT_ROOT/third}"
JOBS="${JOBS:-}"
HIREDIS_DIR="$PROJECT_ROOT/third/redis/deps/hiredis"

# 自动检测并行编译核数
detect_jobs() {
  if [[ -n "${JOBS}" ]]; then
    echo "${JOBS}"
  else
    if command -v nproc >/dev/null 2>&1; then
      nproc
    elif [[ "$OSTYPE" == "darwin"* ]]; then
      sysctl -n hw.logicalcpu
    else
      echo 4
    fi
  fi
}
JOBS="$(detect_jobs)"

# 平台判断
OS="linux"
if [[ "$OSTYPE" == "darwin"* ]]; then
  OS="macos"
fi

# 颜色输出
c_info(){ echo -e "\033[1;34m[INFO]\033[0m $*"; }
c_ok(){ echo -e "\033[1;32m[OK]\033[0m $*"; }
c_warn(){ echo -e "\033[1;33m[WARN]\033[0m $*"; }
c_err(){ echo -e "\033[1;31m[ERR]\033[0m $*"; }

# 依赖检查
need_bin() {
  if ! command -v "$1" >/dev/null 2>&1; then
    c_err "缺少依赖命令：$1"
    exit 1
  fi
}

for bin in git cmake make; do
  need_bin "$bin"
done

# ---------- 公用函数 ----------
ensure_submodule() {
  local path="$1"
  local repo="$2"    # 可为空：如果是项目自带子模块，只需要 init/update
  if [[ -d "$path/.git" ]] || git submodule status "$path" >/dev/null 2>&1; then
    c_info "子模块存在：$path -> 更新"
    git submodule update --init --recursive "$path"
  else
    if [[ -n "$repo" ]]; then
      c_info "添加并初始化子模块：$path"
      git submodule add -f "$repo" "$path" || true
      git submodule update --init --recursive "$path"
    else
      c_info "初始化项目自带子模块：$path"
      git submodule update --init --recursive "$path"
    fi
  fi
}

# ---------- 步骤函数 ----------
step_submodules() {
  c_info "[1] 拉取第三方库子模块"
  git submodule update --init --recursive
  c_ok "子模块初始化完成"
}

step_threadpool() {
  c_info "[ThreadPool] 初始化/更新"
  ensure_submodule "third/ThreadPool" "https://github.com/progschj/ThreadPool.git"
  c_ok "ThreadPool 完成"
}

step_redis() {
  c_info "[redis] 初始化/更新"
  ensure_submodule "third/redis" "https://github.com/redis/redis.git"
  git -C "third/redis" submodule update --init --recursive deps/hiredis || true
  c_info "[redis] 开始编译"
  (cd third/redis && make -j"${JOBS}")
  step_hiredis
  c_ok "redis 编译完成"
}

step_hiredis() {
  set -euo pipefail
  c_info "[hiredis] 编译 hiredis (静态)"

  local PROJECT_ROOT_REAL
  PROJECT_ROOT_REAL="$(realpath -P "${PROJECT_ROOT}")"

  local REDIS_DIR="$PROJECT_ROOT_REAL/third/redis"
  local HIREDIS_REL="deps/hiredis"
  local HIREDIS_DIR_REAL="$REDIS_DIR/$HIREDIS_REL"

  if [[ ! -d "$HIREDIS_DIR_REAL" ]] || [[ ! -d "$HIREDIS_DIR_REAL/.git" ]]; then
    c_info "[hiredis] 初始化 redis 子模块：$HIREDIS_REL"
    git -C "$REDIS_DIR" submodule update --init --recursive "$HIREDIS_REL"
  else
    c_info "[hiredis] 子模块已存在，执行更新"
    git -C "$REDIS_DIR" submodule update --recursive "$HIREDIS_REL" || true
  fi

  local PREFIX="${INSTALL_ROOT}/hiredis"
  rm -rf "$PREFIX" && mkdir -p "$PREFIX"

  pushd "$HIREDIS_DIR_REAL" >/dev/null
    rm -rf build && mkdir build && cd build
    cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
      -DENABLE_SSL=OFF \
      -DENABLE_EXAMPLES=OFF \
      -DENABLE_TESTS=OFF \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      -DCMAKE_INSTALL_PREFIX="$PREFIX" \
      -DCMAKE_INSTALL_LIBDIR=lib
    make -j"${JOBS}"
    make install
  popd >/dev/null

  export HIREDIS_ROOT="$PREFIX"
  export HIREDIS_INCLUDE_DIR="$PREFIX/include"
  export HIREDIS_LIBRARY="$PREFIX/lib/libhiredis.a"

  c_ok "hiredis 安装完成（静态）：$HIREDIS_LIBRARY"
}

step_proto() {
  c_info "[proto] 生成 protobuf"
  if [[ -x "$PROJECT_ROOT/shell/proto.sh" ]]; then
    "$PROJECT_ROOT/shell/proto.sh"
  else
    c_warn "未找到 shell/proto.sh 或不可执行，跳过"
  fi
  c_ok "protobuf 步骤完成（若脚本存在）"
}

# ---------- 任务选择 ----------
ALL_STEPS=(submodules threadpool redis proto)

run_step() {
  case "$1" in
    submodules)  step_submodules ;;
    threadpool)  step_threadpool ;;
    redis)       step_redis ;;
    proto)       step_proto ;;
    *) c_err "未知步骤：$1"; exit 1 ;;
  esac
}

main() {
  c_info "PROJECT_ROOT=$PROJECT_ROOT"
  c_info "INSTALL_ROOT=$INSTALL_ROOT"
  c_info "OS=$OS  JOBS=$JOBS"

  if [[ $# -eq 0 ]]; then
    # 默认顺序与说明保持一致
    SEQ=(submodules threadpool redis proto)
  else
    SEQ=("$@")
  fi

  for s in "${SEQ[@]}"; do
    run_step "$s"
  done

  c_ok "[SUCCESS] 全部完成"
}

main "$@"
