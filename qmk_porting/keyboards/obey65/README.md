# Obey65

CH582M 65% keyboard target for the local QMK port.

## Verified state

- USB HID, VIA, and RGB were previously verified on hardware by the user.
- Current profiles compile; BLE and 2.4 GHz have not been verified on hardware in this audit.
- 2.4 GHz remains deferred and must not be described as implemented.

## Build

From the repository root:

```sh
cmake --preset obey65-wired
cmake --build --preset obey65-wired --parallel
```

BLE profiles, in test order:

```text
obey65-ble-smoke  advertising only
obey65-ble-hid    HOGP service, no QMK
obey65-ble-qmk    HOGP + QMK, no RGB
obey65-ble-dev    integration build
```

Generated files are placed at the repository root with the profile in the filename; intermediate files are under `out/build/`.

## USB diagnostics

With the wired profile and VIA closed:

```sh
python3 tools/obey65_diag.py list
python3 tools/obey65_diag.py hello
python3 tools/obey65_diag.py system
```

Install Python `hidapi` only when using the host tool. See the repository [status](../../../docs/STATUS.md) and [hardware plan](../../../docs/HARDWARE_TEST_PLAN.md).
