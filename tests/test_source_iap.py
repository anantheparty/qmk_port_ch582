import importlib.util
import pathlib
import sys
import unittest


MODULE_PATH = pathlib.Path(__file__).parents[1] / "tools" / "check_source_iap.py"
SPEC = importlib.util.spec_from_file_location("check_source_iap", MODULE_PATH)
checker = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = checker
SPEC.loader.exec_module(checker)


class SourceIapBoundaryTests(unittest.TestCase):
    def test_accepts_standard_mcuboot_path(self):
        symbols = "0001 T boot_go\n0002 T bootloader_set_to_default_mode\n"
        self.assertEqual(checker.validate_symbols(symbols), [])

    def test_rejects_dummy_rsa_and_external_hooks(self):
        symbols = "0001 R rsa_pub_key\n0002 T iap_validate\n"
        errors = checker.validate_symbols(symbols)
        self.assertTrue(any("rsa_pub_key" in error for error in errors))
        self.assertTrue(any("iap_validate" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
