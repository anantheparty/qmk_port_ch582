#!/usr/bin/env python3
"""Obey65 Raw HID diagnostics client."""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from typing import Any, Iterable, Protocol

VID = 0xCAFE
PID = 0x0B97
USAGE_PAGE = 0xFF60
USAGE = 0x61
FRAME_SIZE = 32
COMMAND = 0xD0
MAGIC = 0x65
VERSION = 1

OP_HELLO = 1
OP_GET_SYSTEM = 2

STATUS_NAMES = {
    0: "ok",
    1: "bad_magic",
    2: "bad_version",
    3: "bad_opcode",
}


class Transport(Protocol):
    def write(self, payload: bytes) -> int: ...

    def read(self, size: int, timeout_ms: int) -> bytes | list[int]: ...


@dataclass(frozen=True)
class Response:
    sequence: int
    opcode: int
    status: int
    payload: bytes


def build_request(opcode: int, sequence: int = 1, payload: bytes = b"") -> bytes:
    if len(payload) > FRAME_SIZE - 8:
        raise ValueError("payload exceeds 24 bytes")
    frame = bytearray(FRAME_SIZE)
    frame[0:7] = bytes((COMMAND, MAGIC, VERSION, sequence & 0xFF, opcode & 0xFF, 0, len(payload)))
    frame[8 : 8 + len(payload)] = payload
    return bytes(frame)


def parse_response(raw: bytes | Iterable[int], sequence: int, opcode: int) -> Response:
    frame = bytes(raw)
    if len(frame) == FRAME_SIZE + 1 and frame[0] == 0:
        frame = frame[1:]
    if len(frame) != FRAME_SIZE:
        raise ValueError(f"expected {FRAME_SIZE} bytes, got {len(frame)}")
    if frame[0:3] != bytes((COMMAND, MAGIC, VERSION)):
        raise ValueError("response namespace, magic, or version mismatch")
    if frame[3] != (sequence & 0xFF):
        raise ValueError(f"sequence mismatch: expected {sequence & 0xFF}, got {frame[3]}")
    if frame[4] != (opcode & 0xFF):
        raise ValueError(f"opcode mismatch: expected {opcode & 0xFF}, got {frame[4]}")
    payload_length = frame[6]
    if payload_length > FRAME_SIZE - 8:
        raise ValueError("invalid response payload length")
    return Response(frame[3], frame[4], frame[5], frame[8 : 8 + payload_length])


def transact(transport: Transport, opcode: int, sequence: int = 1, timeout_ms: int = 200) -> Response:
    request = build_request(opcode, sequence)
    written = transport.write(b"\x00" + request)
    if written not in (FRAME_SIZE, FRAME_SIZE + 1):
        raise RuntimeError(f"short HID write: {written}")
    raw = transport.read(FRAME_SIZE, timeout_ms)
    if not raw:
        raise TimeoutError(f"no response in {timeout_ms} ms")
    return parse_response(raw, sequence, opcode)


def _load_hid() -> Any:
    try:
        import hid  # type: ignore
    except ImportError as exc:
        raise SystemExit("Python package 'hidapi' is required: python3 -m pip install hidapi") from exc
    return hid


def list_devices() -> list[dict[str, Any]]:
    hid = _load_hid()
    return [
        item
        for item in hid.enumerate(VID, PID)
        if item.get("usage_page") == USAGE_PAGE and item.get("usage") == USAGE
    ]


def open_device(path: bytes | str | None = None) -> Any:
    hid = _load_hid()
    devices = list_devices()
    if not devices:
        raise SystemExit("Obey65 QMK Raw HID interface not found")
    selected = devices[0] if path is None else next((item for item in devices if item.get("path") == path), None)
    if selected is None:
        raise SystemExit("requested HID path not found")
    device = hid.device()
    device.open_path(selected["path"])
    return device


def decode_hello(response: Response) -> dict[str, Any]:
    if len(response.payload) < 8:
        raise ValueError("short HELLO payload")
    payload = response.payload
    return {
        "status": STATUS_NAMES.get(response.status, f"unknown_{response.status}"),
        "protocol_version": payload[0],
        "capabilities": payload[1],
        "active_protocol": payload[2],
        "stored_boot_mode": payload[3],
        "uptime_ms": int.from_bytes(payload[4:8], "little"),
        "profile": payload[8:].split(b"\0", 1)[0].decode("ascii", errors="replace"),
    }


def decode_system(response: Response) -> dict[str, Any]:
    if len(response.payload) < 10:
        raise ValueError("short GET_SYSTEM payload")
    payload = response.payload
    return {
        "status": STATUS_NAMES.get(response.status, f"unknown_{response.status}"),
        "active_protocol": payload[0],
        "stored_boot_mode": payload[1],
        "usb_state": payload[2],
        "dropped_requests": payload[3],
        "uptime_ms": int.from_bytes(payload[4:8], "little"),
        "keyboard_protocol": payload[8],
        "keyboard_idle": payload[9],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("list", "hello", "system"))
    args = parser.parse_args()

    if args.command == "list":
        safe = [{key: str(value) for key, value in item.items()} for item in list_devices()]
        print(json.dumps(safe, ensure_ascii=False, indent=2))
        return 0

    device = open_device()
    try:
        if args.command == "hello":
            result = decode_hello(transact(device, OP_HELLO))
        else:
            result = decode_system(transact(device, OP_GET_SYSTEM))
        print(json.dumps(result, ensure_ascii=False, indent=2))
        return 0
    finally:
        device.close()


if __name__ == "__main__":
    raise SystemExit(main())
