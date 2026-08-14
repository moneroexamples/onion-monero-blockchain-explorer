#!/usr/bin/env python3
"""Run the authenticated atomic-swap sidecar."""

from __future__ import annotations

import argparse
import os
import secrets
from http.server import ThreadingHTTPServer
from pathlib import Path

from http_api import make_handler
from service import SwapService


def load_token(path: Path) -> str:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        token = path.read_text().strip()
        if not token:
            raise RuntimeError(f"empty token file: {path}")
        return token
    token = secrets.token_urlsafe(32)
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w") as handle:
        handle.write(token + "\n")
    return token


def main() -> None:
    parser = argparse.ArgumentParser(description="COMIT/UnstoppableSwap explorer bridge")
    parser.add_argument("--swap-binary", required=True)
    parser.add_argument("--data-dir", default=str(Path.home() / ".local/share/onion-explorer-asaas"))
    parser.add_argument("--bind", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8117)
    parser.add_argument("--allow-remote", action="store_true")
    args = parser.parse_args()
    if args.bind not in {"127.0.0.1", "::1", "localhost"} and not args.allow_remote:
        parser.error("non-loopback bind requires --allow-remote and a TLS reverse proxy")

    data_dir = Path(args.data_dir).expanduser().resolve()
    data_dir.mkdir(parents=True, exist_ok=True, mode=0o700)
    data_dir.chmod(0o700)
    token_path = data_dir / "api.token"
    token = load_token(token_path)
    service = SwapService(args.swap_binary, str(data_dir / "history.sqlite3"), str(data_dir / "logs"))
    server = ThreadingHTTPServer((args.bind, args.port), make_handler(service, token))
    print(f"Atomic swap UI: http://{args.bind}:{args.port}/")
    print(f"API token: {token_path}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
        service.close()


if __name__ == "__main__":
    main()
