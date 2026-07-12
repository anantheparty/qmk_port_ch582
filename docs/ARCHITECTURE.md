# Obey65 architecture

```text
QMK matrix/keymap
      |
      v
host_driver_t
      |
      +-- USB adapter --> CherryUSB --> USB HID / VIA Raw HID
      |
      +-- BLE adapter --> TMOS --> GAP/GATT HOGP
      |
      `-- ESB adapter --> CH58x RF --> receiver --> USB HID   [deferred]
```

The EEPROM boot byte selects one adapter before its platform initialization. Compiling USB and BLE together does not start both transports.
Source wireless builds use the same MCUboot hash-validation path as wired builds. External wireless IAP hooks are reserved for prebuilt libraries that actually provide them.

## Profiles

| Profile | Runtime purpose |
|---|---|
| `wired` | Frozen USB/VIA/RGB baseline and Raw HID diagnostics. |
| `ble-smoke` | GAP advertising only; discrete B17 status LED. |
| `ble-hid` | GAP/GATT/HOGP and bonding; no QMK or RGB. |
| `ble-qmk` | BLE HOGP plus QMK scanning and reports; no USB/VIA/RGB. |
| `ble-dev` | Final integration build with USB, BLE, VIA, QMK, and RGB compiled. |

`ble-dev` still runs only the adapter selected in EEPROM. The single-mode BLE profiles default safely to BLE when the stored byte is unavailable.
The BLE UI currently exposes slot 0 only; GAPBondMgr may retain bonds, but selectable identities are not implemented.

## Diagnostics

The wired Raw HID command uses QMK usage page `0xFF60`, usage `0x61`, and fixed 32-byte frames. Byte 0 is command `0xD0`; byte 3 is the sequence; byte 4 is the opcode. USB OUT callbacks copy frames; the USB task re-arms endpoints and dispatches VIA, lighting, and diagnostics outside callback context.

The BLE status LED is outside both WS2812 timers: on means advertising or connected, and a 500 ms toggle means a reported initialization error. Off means idle or that execution never reached indicator setup, so it cannot diagnose early clock/BLE-init failure by itself.
