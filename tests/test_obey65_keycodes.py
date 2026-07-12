import json
import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).parents[1]
INFO = ROOT / "qmk_porting/keyboards/obey65/info.json"
HEADER = ROOT / "qmk_porting/keyboards/obey65/wireless_mode.h"


class KeycodeAbiTests(unittest.TestCase):
    def test_via_indices_remain_stable(self):
        entries = json.loads(INFO.read_text())["customKeycodes"]
        self.assertEqual(entries[11]["name"], "USB")
        self.assertEqual(entries[12]["name"], "BLE")
        self.assertTrue(all(entry["name"].startswith("RESERVED_") for entry in entries[13:28]))
        self.assertEqual(entries[28]["name"], "2.4G")
        self.assertEqual(entries[29]["name"], "BAT")

    def test_wireless_aliases_use_board_keycodes(self):
        header = HEADER.read_text()
        expected = {"WL_USB": 11, "WL_BLE0": 12, "WL_ESB": 28, "WL_BATTERY": 29}
        for name, offset in expected.items():
            pattern = rf"#define\s+{name}\s+\(QK_KB_0 \+ {offset}\)"
            self.assertRegex(header, re.compile(pattern))


if __name__ == "__main__":
    unittest.main()
