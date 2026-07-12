# Obey65 decisions

| ID | Decision | Reason | Revisit when |
|---|---|---|---|
| D001 | Keep wired behavior in a separate profile. | It is the only hardware-verified baseline. | A wired regression has an isolated fix. |
| D002 | Split BLE into smoke, HOGP, QMK, and integration profiles. | Each hardware run changes one major ownership boundary. | All BLE gates pass. |
| D003 | Treat `../ch583` as read-only reference and SDK releases as bundles. | Startup, HAL, headers, and the binary library are version-coupled. | A complete V2.13 A/B is prepared. |
| D004 | Default to internal 32 kHz; expose external LSE as an A/B option. | This preserves prior runtime behavior while removing hidden configuration override. | Board crystal presence and stability are measured. |
| D005 | Reuse QMK Raw HID for USB diagnostics. | VIA already exposes the endpoint; a CDC interface is unnecessary. | BLE-only trace data must be retrieved. |
| D006 | Convert QMK NKRO to HOGP 6KRO with rollover. | The current BLE report map is 6KRO; silently dropping NKRO reports is incorrect. | A tested BLE NKRO report map is added. |
| D007 | Defer 2.4 GHz implementation until BLE works. | Current keyboard, receiver, protocol, and schematic each have independent blockers. | BLE QMK input is hardware-verified. |
| D008 | Expose one BLE slot and reserve removed keycode IDs. | The old 4/16-slot UI had no switching implementation; VIA keycodes persist in EEPROM. | Slot identity, erase, persistence, migration, and host tests exist. |
| D009 | Use standard MCUboot for source wireless builds. | The removed hooks were empty and the dummy RSA key could not validate generated images. | A real transform/signing contract and matching build signer exist. |
