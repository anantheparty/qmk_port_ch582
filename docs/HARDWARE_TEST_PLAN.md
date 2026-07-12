# Obey65 hardware test plan

For every run record board revision, profile, firmware filename/hash, scanner/host, changed variable, observed result, and next decision. Do not advance on a build result alone.

## 0. Recovery check

Confirm that holding the boot key exposes the UF2 drive and that the `wired` firmware restores USB mode. Do this before a BLE-only image.

## 1. Official radio baseline

Flash an otherwise untouched WCH Broadcaster/Peripheral example from `../ch583`, configured for CH582M and the board clock. Scan without name or manufacturer filters.

- Visible: radio hardware and official stack path pass; continue.
- Invisible: inspect 32 MHz clock, RF supply, antenna/matching, MAC, and board assembly before QMK work.

## 2. Project advertising

Build and flash `obey65-ble-smoke`. Search first without filters, then for name `Obey65` and manufacturer payload `FF FF BE EF CA`.

- Official visible, smoke invisible: inspect boot mode, TMOS events, BLE init status, clock assumptions, and interrupts.
- Both invisible: hardware remains the primary suspect.

If needed, repeat smoke with only external LSE changed. If still blocked, compare the complete local V2.10 SDK bundle with a complete V2.13 port; do not swap only the archive or startup file.

## 3. HOGP service

Flash `obey65-ble-hid`.

1. Connect and bond from a clean host entry.
2. Inspect service `0x1812`, report map, Report Reference descriptors, and keyboard CCCDs.
3. Disconnect and reconnect; verify CCCDs do not leak between connections.

## 4. QMK input

Flash `obey65-ble-qmk`.

1. Verify one press and its release.
2. Verify modifiers, six simultaneous keys, rollover, media keys, and host LED output.
3. Run continuous typing and reconnect tests before enabling RGB.

## 5. Integration

Flash `obey65-ble-dev`, select BLE through the existing mode key, and repeat step 4. Add RGB last and compare with RGB disabled for TMR1/TMR2 interference. Test power first; design selectable bond slots only after stable input.

## 6. 2.4 GHz entry gate

Before firmware work, review the receiver RF/VINTA/antenna network and add reachable pairing/status controls. Then create separate keyboard/receiver radio-smoke profiles based on the official RF_PHY example: fixed channel, Basic mode, callback only records status and posts a TMOS event. Pairing, ACK/retry, hopping, power, and NKRO follow only after one-way packets and CRC/RSSI are measured.
