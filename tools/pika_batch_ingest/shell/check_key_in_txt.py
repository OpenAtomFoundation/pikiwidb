#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse, json, re, sys
from pathlib import Path
from typing import Set, Tuple, List, Union

KEY_RE   = re.compile(r"(key_\d+)", re.I)
VALUE_RE = re.compile(r"(value_\d+)", re.I)

# 记录分隔符：优先用 '------'；没有时再按空行分组
DASH_SPLIT = re.compile(r"^\s*-{6,}\s*$", re.M)
BLANK_SPLIT = re.compile(r"(?:\r?\n){2,}", re.M)

def load_json_keys(p: Union[str, Path]) -> Set[str]:
    data = json.loads(Path(p).read_text(encoding="utf-8"))
    keys: Set[str] = set()
    if isinstance(data, list):
        for i, item in enumerate(data):
            if isinstance(item, dict):
                if isinstance(item.get("key"), str):
                    keys.add(item["key"])
                for k in item.keys():
                    if isinstance(k, str) and k.lower().startswith("key_"):
                        keys.add(k)
            else:
                raise ValueError(f"JSON 第 {i} 项不是对象：{type(item)}")
    elif isinstance(data, dict):
        for k in data.keys():
            if isinstance(k, str) and k.lower().startswith("key_"):
                keys.add(k)
        if isinstance(data.get("key"), str):
            keys.add(data["key"])
    else:
        raise ValueError(f"不支持的 JSON 顶层类型：{type(data)}")
    return keys

def normalize(s: str) -> str:
    # 把 'k e y _ 1 2 3' 这类拆分字符粘回去；仅保留 [a-z0-9_]
    s = s.lower().replace("\\0", "")
    return re.sub(r"[^a-z0-9_]+", "", s)

def split_records(raw: str) -> List[str]:
    parts = [p for p in DASH_SPLIT.split(raw) if p.strip()]
    if len(parts) <= 1:  # 若没有 '------'，按空行兜底
        parts = [p for p in BLANK_SPLIT.split(raw) if p.strip()]
    return parts

def extract_pairs(record_text: str) -> List[Tuple[str, str]]:
    norm = normalize(record_text)
    ks = KEY_RE.findall(norm)
    vs = VALUE_RE.findall(norm)
    n = min(len(ks), len(vs))
    return list(zip(ks[:n], vs[:n]))

def main():
    ap = argparse.ArgumentParser(
        description="在整份 txt 中，仅统计同一条记录里 key_* 与 value_* 同时出现的对儿，用这些 key 覆盖校验 JSON。"
    )
    ap.add_argument("json_file", help="包含 key 的 JSON 文件路径")
    ap.add_argument("txt_file", help="ASCII/HEX dump 的 txt 文件路径")
    ap.add_argument("--show-extra", action="store_true", help="显示 txt 有效记录里出现、但 JSON 未包含的 key_*")
    ap.add_argument("--show-pairs", action="store_true", help="调试：打印解析到的 (key, value) 对")
    args = ap.parse_args()

    try:
        json_keys = load_json_keys(args.json_file)
    except Exception as e:
        print(f"[错误] 解析 JSON 失败：{e}", file=sys.stderr)
        sys.exit(2)

    try:
        raw = Path(args.txt_file).read_text(encoding="utf-8", errors="ignore")
    except Exception as e:
        print(f"[错误] 读取 TXT 失败：{e}", file=sys.stderr)
        sys.exit(2)

    records = split_records(raw)
    pairs: List[Tuple[str, str]] = []
    for rec in records:
        pairs.extend(extract_pairs(rec))

    if args.show_pairs:
        print("[调试] 解析到的 (key, value) 对：")
        for k, v in pairs[:500]:
            print(f"  {k} :: {v}")
        if len(pairs) > 500:
            print(f"  ... 以及 {len(pairs)-500} 对更多")

    valid_keys: Set[str] = {k for k, _ in pairs}

    missing = sorted(k for k in json_keys if k not in valid_keys)
    extra   = sorted(k for k in valid_keys if k not in json_keys)

    print(f"JSON 键总数：{len(json_keys)}")
    print(f"txt 有效记录中的键总数：{len(valid_keys)}")

    if missing:
        print("\n[缺失] 以下 JSON 键未在任何“key+value 同时出现”的记录里找到：")
        for k in missing[:200]:
            print("  -", k)
        if len(missing) > 200:
            print(f"  ... 以及 {len(missing)-200} 个更多")
    else:
        print("\n[OK] txt 的有效记录覆盖了 JSON 的全部键。")

    if args.show_extra and extra:
        print("\n[额外] txt 有效记录中出现，但不在 JSON 的键：")
        for k in extra[:200]:
            print("  -", k)
        if len(extra) > 200:
            print(f"  ... 以及 {len(extra)-200} 个更多")

    # 退出码：0=全部匹配；1=有缺失或（当指定 --show-extra 时）有多余；2=解析错误
    if missing or (args.show_extra and extra):
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    main()
