#!/usr/bin/env python3
"""Reject source-wireless IAP ELFs that retain external hook or dummy-key paths."""

from __future__ import annotations

import argparse
import pathlib
import shutil
import subprocess

FORBIDDEN = (
    "bootutil_keys",
    "iap_handle_data",
    "iap_handle_new_wireless_chip",
    "iap_validate",
    "rsa_pub_key",
)

REQUIRED = (
    "boot_go",
    "bootloader_set_to_default_mode",
)


def validate_symbols(symbols: str) -> list[str]:
    errors = [f"external wireless IAP symbol retained: {name}" for name in FORBIDDEN if name in symbols]
    errors.extend(f"standard IAP symbol missing: {name}" for name in REQUIRED if name not in symbols)
    return errors


def resolve_nm(value: str | None, elf: pathlib.Path) -> str:
    if value:
        return value
    found = shutil.which("riscv-wch-elf-nm")
    if found:
        return found
    cache = elf.parent / "CMakeCache.txt"
    if cache.exists():
        prefix = "CMAKE_NM:FILEPATH="
        for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
            if line.startswith(prefix):
                return line[len(prefix) :]
    raise SystemExit("riscv-wch-elf-nm not found; pass --nm")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=pathlib.Path)
    parser.add_argument("--nm")
    args = parser.parse_args()

    result = subprocess.run(
        [resolve_nm(args.nm, args.elf), "-C", str(args.elf)],
        check=True,
        capture_output=True,
        text=True,
    )
    errors = validate_symbols(result.stdout)
    if errors:
        print("\n".join(errors))
        return 1
    print("Source IAP symbol boundary: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
