import importlib.util
import pathlib
import sys
import unittest
from unittest import mock


MODULE_PATH = pathlib.Path(__file__).parents[1] / "tools" / "check_ble_profile.py"
SPEC = importlib.util.spec_from_file_location("check_ble_profile", MODULE_PATH)
smoke = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = smoke
SPEC.loader.exec_module(smoke)


class SymbolBoundaryTests(unittest.TestCase):
    def test_accepts_minimal_ble_runtime(self):
        symbols = "\n".join(smoke.COMMON_REQUIRED)
        self.assertEqual(smoke.validate_symbols(symbols), [])

    def test_rejects_qmk_and_ws2812_runtime(self):
        symbols = "\n".join((*smoke.COMMON_REQUIRED, "run_qmk_task", "tmr2_ws2812_init"))
        errors = smoke.validate_symbols(symbols)
        self.assertEqual(len(errors), 2)

    def test_accepts_qmk_without_usb_or_rgb(self):
        symbols = "\n".join((*smoke.COMMON_REQUIRED, *smoke.PROFILE_REQUIRED["qmk"]))
        self.assertEqual(smoke.validate_symbols(symbols, "qmk"), [])

    def test_reads_nm_path_from_build_cache(self):
        import tempfile

        with tempfile.TemporaryDirectory() as directory:
            build = pathlib.Path(directory)
            expected = "/toolchain/bin/riscv-wch-elf-nm"
            (build / "CMakeCache.txt").write_text(f"CMAKE_NM:FILEPATH={expected}\n")
            with mock.patch.object(smoke.shutil, "which", return_value=None):
                self.assertEqual(smoke.resolve_nm(None, build / "obey65.elf"), expected)


if __name__ == "__main__":
    unittest.main()
