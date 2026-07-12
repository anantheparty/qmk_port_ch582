# Obey65 status

Updated: 2026-07-12

| Area | Evidence | State |
|---|---|---|
| USB HID, VIA, RGB | `HARDWARE_VERIFIED` | Previously verified by the user; this audit had no hardware attached. |
| Current wired profile | `BUILD_VERIFIED` | Builds and links; host diagnostics tests pass. |
| Source IAP image path | `BUILD_VERIFIED` | Standard MCUboot symbols and generated BLE image digest validate; boot on hardware pending. |
| USB Raw HID diagnostics | `BUILD_VERIFIED` | Read-only HELLO and system queries implemented; hardware exchange pending. |
| BLE advertising smoke | `BUILD_VERIFIED` | QMK, HOGP, USB, RGB, TMR1, and TMR2 are absent from the linked runtime. |
| BLE HOGP/QMK | `BUILD_VERIFIED` | HOGP service, CCCD lifecycle, report references, QMK task, and NKRO-to-6KRO path compile; no radio or host validation. |
| BLE power and selectable multi-slot | `UNVERIFIED` | Power hooks remain placeholders; the UI now exposes only the implemented slot 0. |
| 2.4 GHz keyboard | `UNVERIFIED` | No supported build profile; RF callback parsing, ACK ownership, pairing, and NKRO path are broken. |
| 2.4 GHz receiver | `BUILD_VERIFIED` | Temporary APP/IAP build passed; RF and USB report delivery are not functional evidence. |

## Audit conclusions

- `startup_CH583.S` and `CH583SFR.h` are CH58x family files and do not by name prove a CH582 error.
- The local BLE bundle is WCH V2.10; `../ch583` contains V2.13. Compare complete bundles only after the baseline radio test.
- A BLE-enabled binary could previously remain in stored USB mode. Single-mode test profiles now force an available mode.
- Source BLE IAP enabled RSA validation with a one-byte dummy key, emitted unsigned images, and used an empty new-chip hook. Source profiles now use the standard hash-validated MCUboot path; hardware boot remains unverified.
- The former smoke path still started WS2812/TMR2. The smoke runtime is now isolated and checked by symbols.
- Advertising encoded `BE EF` while the scanner expected `BE EF CA`; the packet now contains the full marker.
- The device-name call previously read 21 bytes from a short literal; it now uses a fixed-size buffer.
- QMK is not intrinsically USB-only: `host_driver_t` is the transport seam. The prior BLE adapter omitted QMK scheduling, HOGP registration, and NKRO delivery.
- Former `WL_*` values collided with QMK bootloader/reboot/auto-shift keycodes; wireless actions now use only the board-reserved `QK_KB_n` range.
- VIA metadata advertised 16 BLE slots while the transport ignored every slot number. Only slot 0 is exposed; legacy `QK_KB_13..27` values remain reserved so persisted keymaps are not renumbered.
- VIA and lighting OUT frames formerly executed application logic in USB callback context. Callbacks now copy frames and defer dispatch to the USB task.
- The board setting for an external 32 kHz crystal was silently discarded. Internal 32 kHz remains the preserved default; external LSE is an explicit A/B option.
- The receiver schematic must be reviewed before 2.4 GHz work: PA8/PA9 controls are absent, UART pins are unreachable, and the RF/VINTA/antenna network appears suspect.

Next gate: run [HARDWARE_TEST_PLAN.md](HARDWARE_TEST_PLAN.md) from the official broadcaster test.
