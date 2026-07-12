import importlib.util
import pathlib
import sys
import unittest


MODULE_PATH = pathlib.Path(__file__).parents[1] / "tools" / "obey65_diag.py"
SPEC = importlib.util.spec_from_file_location("obey65_diag", MODULE_PATH)
diag = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = diag
SPEC.loader.exec_module(diag)


class FakeTransport:
    def __init__(self, response):
        self.response = response
        self.writes = []

    def write(self, payload):
        self.writes.append(bytes(payload))
        return len(payload)

    def read(self, size, timeout_ms):
        return self.response


class PacketTests(unittest.TestCase):
    def test_request_is_32_byte_raw_hid_frame(self):
        request = diag.build_request(diag.OP_HELLO, sequence=7)
        self.assertEqual(len(request), 32)
        self.assertEqual(request[:7], bytes((0xD0, 0x65, 1, 7, 1, 0, 0)))

    def test_parse_rejects_late_response(self):
        response = bytearray(diag.build_request(diag.OP_HELLO, sequence=2))
        with self.assertRaisesRegex(ValueError, "sequence mismatch"):
            diag.parse_response(response, sequence=1, opcode=diag.OP_HELLO)

    def test_transact_adds_hidapi_report_id(self):
        response = bytearray(32)
        response[:7] = bytes((0xD0, 0x65, 1, 3, 1, 0, 0))
        transport = FakeTransport(response)
        parsed = diag.transact(transport, diag.OP_HELLO, sequence=3)
        self.assertEqual(parsed.status, 0)
        self.assertEqual(len(transport.writes[0]), 33)
        self.assertEqual(transport.writes[0][0], 0)

    def test_parse_rejects_oversized_payload(self):
        response = bytearray(32)
        response[:7] = bytes((0xD0, 0x65, 1, 1, 1, 0, 25))
        with self.assertRaisesRegex(ValueError, "payload length"):
            diag.parse_response(response, sequence=1, opcode=diag.OP_HELLO)


if __name__ == "__main__":
    unittest.main()
