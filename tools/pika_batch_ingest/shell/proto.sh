#!/usr/bin/env bash
set -euo pipefail

# 计算路径：proto.sh -> ../../.. 就是仓库根 /home/ospp/work/pikiwidb
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"   # …/pikiwidb

# 可选：加载你的 env.sh（如果里面导出了 PROTO_DIR / cd_proto 等）
if [[ -f "${SCRIPT_DIR}/env.sh" ]]; then
  # shellcheck disable=SC1090
  source "${SCRIPT_DIR}/env.sh"
fi

# 如果 env.sh 里定义了 cd_proto，就先切到 proto 目录；否则从变量或默认推断
if type -t cd_proto >/dev/null 2>&1; then
  cd_proto
fi

# 确定 PROTO_DIR（优先环境变量；否则用默认：tools/pika_batch_ingest/src/s3put/include/proto）
: "${PROTO_DIR:="${REPO_ROOT}/tools/pika_batch_ingest/src/s3put/include/proto"}"
OUT_DIR="${PROTO_DIR}"

echo "[DEBUG] 仓库根：${REPO_ROOT}"
echo "[DEBUG] 编译目录：${PROTO_DIR}"
echo "[DEBUG] 输出目录：${OUT_DIR}"

# 选择 protoc：优先使用仓库里的 deps/bin/protoc（相对路径），否则退回系统 protoc
if [[ -x "${REPO_ROOT}/deps/bin/protoc" ]]; then
  PROTOC="${REPO_ROOT}/deps/bin/protoc"
else
  PROTOC="$(command -v protoc || true)"
fi
if [[ -z "${PROTOC}" ]]; then
  echo "❌ 找不到 protoc，可在 ${REPO_ROOT}/deps/bin/protoc 放置对应版本的 protoc" >&2
  exit 1
fi

echo "[DEBUG] 使用 protoc：${PROTOC} ($("${PROTOC}" --version))"

# 创建输出目录
mkdir -p "${OUT_DIR}"

# 编译所有 .proto（稳健处理空目录与空格路径）
shopt -s nullglob
found_any=false
for proto_file in "${PROTO_DIR}"/*.proto; do
  found_any=true
  echo "Compiling ${proto_file} ..."
  # IMPORT 根指向 PROTO_DIR，使得 #include \"proto/xxx.pb.h\" 的相对层级一致
  "${PROTOC}" \
    --proto_path="${PROTO_DIR}" \
    --cpp_out="${OUT_DIR}" \
    "${proto_file}"
done
shopt -u nullglob

if [[ "${found_any}" == false ]]; then
  echo "⚠️  在 ${PROTO_DIR} 下未找到任何 .proto 文件"
fi

echo "✅ All .proto files compiled to ${OUT_DIR}"
