import json
import os
import stat
import tempfile
import threading
import unittest
import urllib.error
import urllib.request
from http.server import ThreadingHTTPServer
from pathlib import Path

from http_api import make_handler
from service import SwapService


FAKE_SWAP = "#!/usr/bin/env python3\nprint('{}', flush=True)\n"


class HttpApiTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        binary = root / "swap"
        binary.write_text(FAKE_SWAP, encoding="utf-8")
        binary.chmod(binary.stat().st_mode | stat.S_IEXEC)
        self.service = SwapService(str(binary), str(root / "db.sqlite3"))
        self.server = ThreadingHTTPServer(("127.0.0.1", 0), make_handler(self.service, "secret"))
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.base = f"http://127.0.0.1:{self.server.server_port}"

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()
        self.service.close()
        self.tmp.cleanup()

    def request(self, method, path, payload=None, token="secret"):
        data = None if payload is None else json.dumps(payload).encode()
        headers = {"X-ASAAS-Token": token, "Content-Type": "application/json"}
        req = urllib.request.Request(self.base + path, data=data, headers=headers, method=method)
        with urllib.request.urlopen(req, timeout=2) as response:
            return response.status, json.load(response)

    def test_start_and_history_end_to_end(self):
        status, created = self.request("POST", "/api/swaps", {
            "seller": "seller",
            "receive_address": "xmr-address",
            "change_address": "btc-address",
        })
        self.assertEqual(status, 202)
        status, history = self.request("GET", "/api/swaps")
        self.assertEqual(status, 200)
        self.assertEqual(history[0]["id"], created["id"])
        self.service.close()
        status, log = self.request("GET", f"/api/actions/{created['id']}/log")
        self.assertEqual(status, 200)
        self.assertIn("output", log)

    def test_resume_and_cancel_end_to_end(self):
        swap_id = "6ba7b810-9dad-11d1-80b4-00c04fd430c8"
        self.assertEqual(self.request("POST", f"/api/swaps/{swap_id}/resume", {})[0], 202)
        self.assertEqual(self.request("POST", f"/api/swaps/{swap_id}/cancel", {})[0], 202)

    def test_protocol_history_end_to_end(self):
        status, result = self.request("GET", "/api/protocol-history")
        self.assertEqual(status, 200)
        self.assertIn("output", result)

    def test_mutation_requires_token(self):
        with self.assertRaises(urllib.error.HTTPError) as caught:
            self.request("POST", "/api/swaps", {}, token="wrong")
        self.assertEqual(caught.exception.code, 403)

    def test_ui_is_served_without_exposing_token(self):
        with urllib.request.urlopen(self.base + "/", timeout=2) as response:
            body = response.read().decode()
        self.assertIn("Atomic Swap", body)
        self.assertNotIn("secret", body)


if __name__ == "__main__":
    unittest.main()
