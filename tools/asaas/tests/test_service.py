import json
import os
import stat
import tempfile
import time
import unittest
from pathlib import Path

from service import SwapService


FAKE_SWAP = r'''#!/usr/bin/env python3
import json, os, sys, time
with open(os.environ["FAKE_SWAP_LOG"], "a", encoding="utf-8") as fh:
    fh.write(json.dumps(sys.argv[1:]) + "\n")
print(json.dumps({"state": "started", "args": sys.argv[1:]}), flush=True)
'''


class SwapServiceTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.binary = root / "swap"
        self.binary.write_text(FAKE_SWAP, encoding="utf-8")
        self.binary.chmod(self.binary.stat().st_mode | stat.S_IEXEC)
        self.log = root / "calls.jsonl"
        self.old_log = os.environ.get("FAKE_SWAP_LOG")
        os.environ["FAKE_SWAP_LOG"] = str(self.log)
        self.service = SwapService(str(self.binary), str(root / "swaps.sqlite3"))

    def tearDown(self):
        self.service.close()
        if self.old_log is None:
            os.environ.pop("FAKE_SWAP_LOG", None)
        else:
            os.environ["FAKE_SWAP_LOG"] = self.old_log
        self.tmp.cleanup()

    def calls(self):
        deadline = time.time() + 2
        while time.time() < deadline:
            if self.log.exists() and self.log.read_text(encoding="utf-8").strip():
                return [json.loads(x) for x in self.log.read_text(encoding="utf-8").splitlines()]
            time.sleep(0.02)
        return []

    def test_start_swap_executes_binary_without_shell_and_records_history(self):
        record = self.service.start(
            seller="/dns4/example.test/tcp/9939/p2p/12D3KooWabc",
            receive_address="48A1ValidMoneroAddressForTest",
            change_address="bc1qchangeaddressfortest",
        )
        self.assertEqual(record["action"], "start")
        self.assertEqual(record["status"], "running")
        self.assertEqual(self.calls()[0], [
            "buy-xmr", "--seller", "/dns4/example.test/tcp/9939/p2p/12D3KooWabc",
            "--receive-address", "48A1ValidMoneroAddressForTest", "--change-address", "bc1qchangeaddressfortest",
        ])
        self.assertEqual(self.service.history()[0]["id"], record["id"])

    def test_resume_uses_validated_swap_id(self):
        swap_id = "6ba7b810-9dad-11d1-80b4-00c04fd430c8"
        record = self.service.resume(swap_id)
        self.assertEqual(record["swap_id"], swap_id)
        self.assertEqual(self.calls()[0], ["resume", "--swap-id", swap_id])

    def test_cancel_requests_protocol_refund_not_process_kill(self):
        swap_id = "6ba7b810-9dad-11d1-80b4-00c04fd430c8"
        record = self.service.cancel(swap_id)
        self.assertEqual(record["action"], "cancel")
        self.assertEqual(self.calls()[0], ["cancel-and-refund", "--swap-id", swap_id])

    def test_protocol_history_runs_the_swap_history_command(self):
        output = self.service.protocol_history()
        self.assertIn('"state": "started"', output)
        self.assertEqual(self.calls()[0], ["history"])

    def test_action_log_exposes_swap_quote_and_deposit_output(self):
        record = self.service.start("seller", "xmr", "btc")
        self.service.close()
        self.assertIn('"state": "started"', self.service.action_log(record["id"]))

    def test_sensitive_state_uses_owner_only_permissions(self):
        database = Path(self.service.database)
        self.assertEqual(stat.S_IMODE(database.stat().st_mode), 0o600)
        self.assertEqual(stat.S_IMODE(self.service.log_directory.stat().st_mode), 0o700)

    def test_invalid_inputs_are_rejected_before_execution(self):
        with self.assertRaisesRegex(ValueError, "seller"):
            self.service.start("seller\n--evil", "xmr", "btc")
        with self.assertRaisesRegex(ValueError, "swap id"):
            self.service.resume("; rm -rf /")
        self.assertEqual(self.calls(), [])


if __name__ == "__main__":
    unittest.main()
