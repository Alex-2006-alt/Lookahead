#!/usr/bin/env python3
"""
Architecture Layer & Code Quality Verifier for Lookahead.

Enforces:
1. Strict one-way layer dependencies:
   core <- movegen <- eval <- search <- interface
2. Maximum file length limit (<= 300 lines).
3. No forbidden generic file names (e.g., utils, misc, helper).
4. Mandatory 3-6 line explanatory comment block at the top of every C++ source/header.
"""

import sys
import re
from pathlib import Path

# Hierarchy definition: lower rank cannot include higher rank.
# 0: core
# 1: movegen
# 2: eval
# 3: search
# 4: interface
LAYER_RANKS = {
    "core": 0,
    "movegen": 1,
    "eval": 2,
    "search": 3,
    "interface": 4,
}

FORBIDDEN_NAME_PATTERNS = [
    r"util",
    r"misc",
    r"helper",
    r"common",
]

MAX_LINES = 300


def check_file(file_path: Path, src_root: Path) -> list[str]:
    violations = []
    rel_path = file_path.relative_to(src_root)
    parts = rel_path.parts

    if len(parts) < 2:
        # File at root of src (e.g. main.cpp)
        file_layer = None
    else:
        file_layer = parts[0]

    # 1. Check file naming
    stem_lower = file_path.stem.lower()
    for pattern in FORBIDDEN_NAME_PATTERNS:
        if re.search(pattern, stem_lower):
            violations.append(
                f"{file_path}: Forbidden generic file name '{file_path.name}'. "
                f"Each file must represent one concrete domain concept."
            )

    try:
        content = file_path.read_text(encoding="utf-8")
    except Exception as e:
        violations.append(f"{file_path}: Unable to read file: {e}")
        return violations

    lines = content.splitlines()

    # 2. Check file line length limit
    if len(lines) > MAX_LINES:
        violations.append(
            f"{file_path}: Exceeds maximum line limit of {MAX_LINES} lines "
            f"(currently {len(lines)} lines)."
        )

    # 3. Check for leading comment header (only for C++ files with code)
    if lines:
        header_comment = False
        # Look for /* ... */ or consecutive // comments in first 10 lines
        comment_lines = 0
        in_block_comment = False
        for line in lines[:10]:
            stripped = line.strip()
            if stripped.startswith("/*"):
                in_block_comment = True
                comment_lines += 1
            elif in_block_comment:
                comment_lines += 1
                if "*/" in stripped:
                    in_block_comment = False
            elif stripped.startswith("//"):
                comment_lines += 1
            elif not stripped:
                continue
            else:
                break

        if comment_lines < 3:
            violations.append(
                f"{file_path}: Missing mandatory 3 to 6 line leading comment header "
                f"(explaining what it does, why it exists, and how it fits into layers). "
                f"Found {comment_lines} comment lines."
            )

    # 4. Check layer include hierarchy
    include_regex = re.compile(r'#include\s+["<]([^">]+)[">]')
    if file_layer and file_layer in LAYER_RANKS:
        current_rank = LAYER_RANKS[file_layer]
        for idx, line in enumerate(lines, start=1):
            match = include_regex.search(line)
            if not match:
                continue
            inc_target = match.group(1).replace("\\", "/")
            inc_parts = inc_target.split("/")
            
            # Check if including another layer
            if len(inc_parts) >= 2:
                target_layer = inc_parts[0]
                if target_layer in LAYER_RANKS:
                    target_rank = LAYER_RANKS[target_layer]
                    if target_rank > current_rank:
                        violations.append(
                            f"{file_path}:{idx}: Layer violation! "
                            f"Layer '{file_layer}' (rank {current_rank}) cannot include "
                            f"'{target_layer}' (rank {target_rank}). "
                            f"Dependency direction must be: core <- movegen <- eval <- search <- interface."
                        )

    return violations


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent
    src_root = repo_root / "engine" / "src"

    if not src_root.exists():
        print(f"[check_layers] engine/src does not exist yet at {src_root}. Skipping.")
        return 0

    cpp_extensions = {".h", ".hpp", ".cpp", ".cxx", ".cc"}
    all_files = [
        p for p in src_root.rglob("*")
        if p.is_file() and p.suffix.lower() in cpp_extensions
    ]

    all_violations = []
    for file_path in all_files:
        violations = check_file(file_path, src_root)
        all_violations.extend(violations)

    if all_violations:
        print(f"[check_layers] Found {len(all_violations)} violation(s):")
        for v in all_violations:
            print(f"  - {v}")
        return 1

    print(f"[check_layers] All {len(all_files)} files pass layer and style checks successfully.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
