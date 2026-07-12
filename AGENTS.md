# Obey65 agent workflow

## Scope

Work on Obey65 in `qmk_porting/keyboards/obey65` and the CH58x adapter around it. Keep `qmk_firmware`, `mcuboot`, and other submodules unchanged unless the user explicitly expands scope.

Source authority, highest first:

1. Observed hardware behavior.
2. Official WCH material in read-only `../ch583`.
3. The pinned SDK/QMK version in this repository.
4. Historical project notes and commit messages.

Never replace one file from a WCH SDK release in isolation. Treat BLE library, HAL, headers, and startup as one versioned bundle.
Source-built wireless profiles use the standard MCUboot image path. Do not restore external wireless IAP hooks or RSA signing without a real implementation, public key, and matching post-build signer.
Treat existing `QK_KB_n` values as persisted VIA ABI. Reserve removed entries; do not compact or reuse their numbers.

## Evidence

Use only these status labels:

- `HARDWARE_VERIFIED`: observed on named hardware with recorded firmware/profile.
- `BUILD_VERIFIED`: clean configure, compile, link, and relevant host tests passed.
- `UNVERIFIED`: implemented or inferred, without the required observation.
- `BLOCKED`: a stated external dependency prevents the next gate.

A successful build is not hardware completion. Commit messages and old checklists are not test evidence.

## Priority gates

Do not start a later gate before the earlier gate has evidence.

1. Preserve the wired USB/VIA/RGB baseline.
2. Make the untouched official WCH broadcaster visible on Obey65 hardware.
3. Make `ble-smoke` visible with QMK, HID, VIA, and WS2812 absent.
4. Validate `ble-hid` discovery, connection, HOGP descriptors, and bonding.
5. Validate `ble-qmk` press and release reports without RGB.
6. Validate `ble-dev`, then power management and multiple bonds.
7. Repair and validate 2.4 GHz with separate radio-smoke targets.

## Work loop

1. Inspect `git status`; preserve user changes and unrelated submodule dirt.
2. State the hypothesis and select one build profile.
3. Change one ownership boundary or experimental variable.
4. Run host tests and the selected clean build.
5. For hardware work, record profile, firmware hash, one changed variable, observation, and next decision.
6. Update only canonical documents under `docs/`; remove disproved claims.

Stop and ask before flashing, erasing, changing hardware, or mutating `../ch583`.

## Builds

```sh
cmake --preset obey65-wired
cmake --build --preset obey65-wired --parallel

cmake --preset obey65-ble-smoke
cmake --build --preset obey65-ble-smoke --parallel

cmake --preset obey65-ble-hid
cmake --build --preset obey65-ble-hid --parallel

cmake --preset obey65-ble-qmk
cmake --build --preset obey65-ble-qmk --parallel

cmake --preset obey65-ble-dev
cmake --build --preset obey65-ble-dev --parallel

python3 -m unittest discover -s tests -p 'test_*.py' -v
python3 tools/check_ble_profile.py out/build/obey65-ble-smoke/obey65.elf
python3 tools/check_ble_profile.py --profile hid out/build/obey65-ble-hid/obey65.elf
python3 tools/check_ble_profile.py --profile qmk out/build/obey65-ble-qmk/obey65.elf
python3 tools/check_source_iap.py out/build/obey65-ble-smoke/obey65_IAP.elf
python3 mcuboot/scripts/imgtool.py verify out/build/obey65-ble-smoke/obey65_signed.hex
```

Builds go under `out/build/`. Do not reuse `build_ble/` as evidence. Profile names must remain in generated firmware filenames.

## Debugging

Assume no usable UART. In USB mode use the existing 32-byte QMK Raw HID interface and `tools/obey65_diag.py`.

- USB OUT callbacks may only validate and copy a frame; the USB task re-arms the endpoint before dispatch.
- Execute commands and send replies from the QMK task loop.
- Keep diagnostics versioned, bounded, read-only by default, and one request at a time.
- BLE-only failures need a discrete GPIO code, retained trace, or a dedicated coexistence build; USB Raw HID cannot observe a BLE-only boot live.

For BLE experiments, scan without name or manufacturer filters first. `ble-smoke` must retain no QMK, HOGP, USB, RGB, TMR1, or TMR2 runtime symbols.

## Documentation

Canonical files are `docs/STATUS.md`, `docs/ARCHITECTURE.md`, `docs/DECISIONS.md`, and `docs/HARDWARE_TEST_PLAN.md`.

Keep entries factual and short. Record decisions, evidence, interfaces, and unresolved gates; omit tutorials, estimates, narrative summaries, and facts directly evident from code.
