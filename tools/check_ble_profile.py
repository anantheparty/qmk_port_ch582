#!/usr/bin/env python3
"""Check linked ownership boundaries for Obey65 BLE test profiles."""

from __future__ import annotations

import argparse
import pathlib
import shutil
import subprocess

COMMON_FORBIDDEN = (
    "rgb_matrix_",
    "tmr1_ws2812_",
    "tmr2_ws2812_",
    "usb_task_init",
)

COMMON_REQUIRED = (
    "GAPRole_PeripheralInit",
    "GAPRole_PeripheralStartDevice",
    "TMOS_SystemProcess",
    "ble_TaskProcessEvent",
)

PROFILE_FORBIDDEN = {
    "smoke": ("HidDev_", "protocol_keyboard_task", "run_qmk_task"),
    "hid": ("protocol_keyboard_task", "run_qmk_task"),
    "qmk": (),
}

PROFILE_REQUIRED = {
    "smoke": (),
    "hid": ("HidDev_Report",),
    "qmk": ("HidDev_Report", "protocol_keyboard_task"),
}


def validate_symbols(symbols: str, profile: str = "smoke") -> list[str]:
    forbidden = COMMON_FORBIDDEN + PROFILE_FORBIDDEN[profile]
    required = COMMON_REQUIRED + PROFILE_REQUIRED[profile]
    errors = [f"forbidden symbol retained: {name}" for name in forbidden if name in symbols]
    errors.extend(f"required symbol missing: {name}" for name in required if name not in symbols)
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
    parser.add_argument("--profile", choices=tuple(PROFILE_FORBIDDEN), default="smoke")
    args = parser.parse_args()

    result = subprocess.run(
        [resolve_nm(args.nm, args.elf), "-C", str(args.elf)],
        check=True,
        capture_output=True,
        text=True,
    )
    errors = validate_symbols(result.stdout, args.profile)
    if errors:
        print("\n".join(errors))
        return 1
    print(f"BLE {args.profile} symbol boundary: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
