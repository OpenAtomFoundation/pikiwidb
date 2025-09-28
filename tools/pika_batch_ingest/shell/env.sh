#!/bin/bash
# env.sh —— 所有脚本共享的路径配置

# 项目根目录（相对本脚本：../）
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_BIN_DIR="$PROJECT_ROOT/build/bin"
PROTO_DIR="$PROJECT_ROOT/src/s3put/include/proto"
DICT_JSON="$PROJECT_ROOT/config/dics.json"


export PROJECT_ROOT
export BUILD_BIN_DIR
export PROTO_DIR
export DICT_JSON

cd_project_root() {
  cd "$PROJECT_ROOT" || { echo "❌ 进入项目根失败"; exit 1; }
}
cd_build_bin() {
  cd "$BUILD_BIN_DIR" || { echo "❌ 进入构建二进制目录失败"; exit 1; }
}
cd_proto() {
  cd "$PROTO_DIR" || { echo "❌ 进入 proto 目录失败"; exit 1; }
}
cd_dict_json() {
  cd "$(dirname "$DICT_JSON")" || { echo "❌ 进入字典 JSON 目录失败"; exit 1; }
}