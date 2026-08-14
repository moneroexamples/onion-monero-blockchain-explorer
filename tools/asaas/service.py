"""Safe process and history bridge for the COMIT/UnstoppableSwap CLI."""

from __future__ import annotations

import json
import os
import sqlite3
import subprocess
import threading
import uuid
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


class SwapService:
    def __init__(self, binary: str, database: str, log_directory: str | None = None):
        self.binary = str(Path(binary).expanduser().resolve())
        if not os.path.isfile(self.binary) or not os.access(self.binary, os.X_OK):
            raise ValueError("swap binary must be an executable file")
        self.database = str(Path(database).expanduser().resolve())
        self.log_directory = Path(log_directory or Path(self.database).with_suffix(".logs"))
        self.log_directory.mkdir(parents=True, exist_ok=True, mode=0o700)
        self.log_directory.chmod(0o700)
        Path(self.database).parent.mkdir(parents=True, exist_ok=True)
        self._lock = threading.Lock()
        self._threads: list[threading.Thread] = []
        self._init_database()

    def _connect(self) -> sqlite3.Connection:
        connection = sqlite3.connect(self.database)
        connection.row_factory = sqlite3.Row
        return connection

    def _init_database(self) -> None:
        with self._connect() as db:
            db.execute(
                """CREATE TABLE IF NOT EXISTS swap_actions (
                    id TEXT PRIMARY KEY,
                    swap_id TEXT,
                    action TEXT NOT NULL,
                    command_json TEXT NOT NULL,
                    status TEXT NOT NULL,
                    pid INTEGER,
                    exit_code INTEGER,
                    created_at TEXT NOT NULL,
                    updated_at TEXT NOT NULL,
                    log_path TEXT NOT NULL
                )"""
            )
        os.chmod(self.database, 0o600)

    @staticmethod
    def _validate_text(name: str, value: str, maximum: int = 512) -> str:
        if not isinstance(value, str) or not value or len(value) > maximum:
            raise ValueError(f"invalid {name}")
        if any(ord(character) < 32 or ord(character) == 127 for character in value):
            raise ValueError(f"invalid {name}")
        return value

    @staticmethod
    def _validate_swap_id(value: str) -> str:
        try:
            parsed = uuid.UUID(value)
        except (ValueError, TypeError, AttributeError) as error:
            raise ValueError("invalid swap id") from error
        return str(parsed)


    def _launch(self, action: str, arguments: list[str], swap_id: str | None = None) -> dict[str, Any]:
        action_id = str(uuid.uuid4())
        log_path = self.log_directory / f"{action_id}.jsonl"
        command = [self.binary, *arguments]
        now = datetime.now(timezone.utc).isoformat()
        log_handle = log_path.open("ab", buffering=0)
        try:
            process = subprocess.Popen(
                command,
                stdin=subprocess.DEVNULL,
                stdout=log_handle,
                stderr=subprocess.STDOUT,
                shell=False,
                start_new_session=True,
                close_fds=True,
            )
        except Exception:
            log_handle.close()
            raise
        with self._lock, self._connect() as db:
            db.execute(
                "INSERT INTO swap_actions VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                (action_id, swap_id, action, json.dumps(command), "running", process.pid,
                 None, now, now, str(log_path)),
            )
        thread = threading.Thread(
            target=self._record_exit,
            args=(action_id, process, log_handle),
            daemon=True,
        )
        self._threads.append(thread)
        thread.start()
        return self.get(action_id)

    def close(self) -> None:
        """Wait for launched CLI processes so state and log writes are durable."""
        for thread in self._threads:
            thread.join()
        self._threads.clear()

    def _record_exit(self, action_id: str, process: subprocess.Popen[bytes], log_handle: Any) -> None:
        exit_code = process.wait()
        log_handle.close()
        status = "completed" if exit_code == 0 else "failed"
        now = datetime.now(timezone.utc).isoformat()
        with self._lock, self._connect() as db:
            db.execute(
                "UPDATE swap_actions SET status = ?, exit_code = ?, updated_at = ? WHERE id = ?",
                (status, exit_code, now, action_id),
            )

    def start(self, seller: str, receive_address: str, change_address: str) -> dict[str, Any]:
        seller = self._validate_text("seller", seller)
        receive_address = self._validate_text("receive address", receive_address, 256)
        change_address = self._validate_text("change address", change_address, 256)
        return self._launch("start", [
            "buy-xmr", "--seller", seller,
            "--receive-address", receive_address, "--change-address", change_address,
        ])

    def resume(self, swap_id: str) -> dict[str, Any]:
        swap_id = self._validate_swap_id(swap_id)
        return self._launch("resume", ["resume", "--swap-id", swap_id], swap_id)

    def cancel(self, swap_id: str) -> dict[str, Any]:
        swap_id = self._validate_swap_id(swap_id)
        return self._launch(
            "cancel", ["cancel-and-refund", "--swap-id", swap_id], swap_id
        )

    def protocol_history(self) -> str:
        """Return the swap binary's authoritative swap history."""
        result = subprocess.run(
            [self.binary, "history"],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            shell=False,
            timeout=30,
            check=False,
        )
        if result.returncode != 0:
            raise RuntimeError(f"swap history exited {result.returncode}: {result.stdout[-2000:]}")
        return result.stdout

    def get(self, action_id: str) -> dict[str, Any]:
        action_id = self._validate_swap_id(action_id)
        with self._connect() as db:
            row = db.execute("SELECT * FROM swap_actions WHERE id = ?", (action_id,)).fetchone()
        if row is None:
            raise KeyError(action_id)
        result = dict(row)
        result["command"] = json.loads(result.pop("command_json"))
        return result

    def action_log(self, action_id: str, maximum_bytes: int = 1_048_576) -> str:
        """Read the tail of an action log containing quotes and deposit instructions."""
        record = self.get(action_id)
        path = Path(record["log_path"]).resolve()
        if path.parent != self.log_directory.resolve():
            raise RuntimeError("action log escaped configured log directory")
        if not path.exists():
            return ""
        with path.open("rb") as handle:
            size = path.stat().st_size
            handle.seek(max(0, size - maximum_bytes))
            return handle.read(maximum_bytes).decode("utf-8", errors="replace")

    def history(self, limit: int = 100) -> list[dict[str, Any]]:
        limit = max(1, min(int(limit), 1000))
        with self._connect() as db:
            rows = db.execute(
                "SELECT * FROM swap_actions ORDER BY created_at DESC LIMIT ?", (limit,)
            ).fetchall()
        result = []
        for row in rows:
            item = dict(row)
            item["command"] = json.loads(item.pop("command_json"))
            result.append(item)
        return result
