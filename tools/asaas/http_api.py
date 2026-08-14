"""Loopback HTTP API and small browser UI for SwapService."""

from __future__ import annotations

import hmac
import json
import re
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler
from typing import Any
from urllib.parse import urlparse

from service import SwapService


UI = r'''<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width">
<title>BTC/XMR Atomic Swap</title>
<style>body{font-family:sans-serif;max-width:960px;margin:2rem auto;padding:0 1rem;background:#111;color:#eee}input{width:100%;padding:.55rem;margin:.25rem 0 .8rem;box-sizing:border-box}button{padding:.55rem 1rem;margin:.2rem}section{border:1px solid #444;padding:1rem;margin:1rem 0}pre{white-space:pre-wrap;overflow-wrap:anywhere}.error{color:#ff8b8b}</style></head>
<body><h1>BTC/XMR Atomic Swap</h1><p>Local COMIT/UnstoppableSwap bridge. Funds remain controlled by the swap protocol.</p>
<section><label>Local API token<input id="token" type="password" autocomplete="off"></label></section>
<section><h2>Start swap</h2><p>The swap binary returns a deposit address and quote; send only an amount within the seller's advertised limits.</p><form id="start"><label>Seller multiaddress<input name="seller" required></label><label>XMR receive address<input name="receive_address" required></label><label>BTC change address<input name="change_address" required></label><button>Start</button></form></section>
<section><h2>Resume or cancel/refund</h2><form id="manage"><label>Swap UUID<input name="swap_id" required></label><button name="action" value="resume">Resume</button><button name="action" value="cancel">Cancel and refund</button></form></section>
<section><h2>History and live output</h2><label>Action UUID<input id="action-id"></label><button id="action-log">Action output</button><button id="refresh">Bridge actions</button><button id="protocol">Protocol swaps</button><pre id="output"></pre></section>
<script>
const out=document.querySelector('#output');
async function call(path, method='GET', body=null){const r=await fetch(path,{method,headers:{'Content-Type':'application/json','X-ASAAS-Token':document.querySelector('#token').value},body:body?JSON.stringify(body):null});const j=await r.json();if(!r.ok)throw Error(j.error||r.statusText);return j}
async function history(){try{out.textContent=JSON.stringify(await call('/api/swaps'),null,2)}catch(e){out.textContent=e.message;out.className='error'}}
document.querySelector('#start').onsubmit=async e=>{e.preventDefault();try{const a=await call('/api/swaps','POST',Object.fromEntries(new FormData(e.target)));document.querySelector('#action-id').value=a.id;setTimeout(actionLog,300)}catch(x){out.textContent=x.message}};
document.querySelector('#manage').onsubmit=async e=>{e.preventDefault();const f=new FormData(e.target);const action=e.submitter.value;try{await call('/api/swaps/'+encodeURIComponent(f.get('swap_id'))+'/'+action,'POST',{});await history()}catch(x){out.textContent=x.message}};
document.querySelector('#refresh').onclick=history;
document.querySelector('#protocol').onclick=async()=>{try{out.textContent=(await call('/api/protocol-history')).output}catch(e){out.textContent=e.message}};
async function actionLog(){try{const id=document.querySelector('#action-id').value;out.textContent=(await call('/api/actions/'+encodeURIComponent(id)+'/log')).output}catch(e){out.textContent=e.message}}
document.querySelector('#action-log').onclick=actionLog;
</script></body></html>'''


def make_handler(service: SwapService, token: str) -> type[BaseHTTPRequestHandler]:
    if not token:
        raise ValueError("API token must not be empty")

    class Handler(BaseHTTPRequestHandler):
        server_version = "asaas/1"

        def log_message(self, format: str, *args: Any) -> None:
            return

        def _send_json(self, status: int, payload: Any) -> None:
            encoded = json.dumps(payload, separators=(",", ":")).encode()
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(encoded)))
            self.send_header("Cache-Control", "no-store")
            self.send_header("X-Content-Type-Options", "nosniff")
            self.end_headers()
            self.wfile.write(encoded)

        def _authorized(self) -> bool:
            supplied = self.headers.get("X-ASAAS-Token", "")
            return hmac.compare_digest(supplied.encode(), token.encode())

        def _payload(self) -> dict[str, Any]:
            length = int(self.headers.get("Content-Length", "0"))
            if length > 16_384:
                raise ValueError("request body too large")
            value = json.loads(self.rfile.read(length) or b"{}")
            if not isinstance(value, dict):
                raise ValueError("JSON object required")
            return value

        def do_GET(self) -> None:
            path = urlparse(self.path).path
            if path == "/":
                encoded = UI.encode()
                self.send_response(HTTPStatus.OK)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Content-Length", str(len(encoded)))
                self.send_header("Content-Security-Policy", "default-src 'self'; style-src 'unsafe-inline'; script-src 'unsafe-inline'")
                self.send_header("Cache-Control", "no-store")
                self.end_headers()
                self.wfile.write(encoded)
                return
            if path == "/api/swaps":
                if not self._authorized():
                    self._send_json(HTTPStatus.FORBIDDEN, {"error": "forbidden"})
                    return
                self._send_json(HTTPStatus.OK, service.history())
                return
            if path == "/api/protocol-history":
                if not self._authorized():
                    self._send_json(HTTPStatus.FORBIDDEN, {"error": "forbidden"})
                    return
                try:
                    self._send_json(HTTPStatus.OK, {"output": service.protocol_history()})
                except (RuntimeError, TimeoutError) as error:
                    self._send_json(HTTPStatus.BAD_GATEWAY, {"error": str(error)})
                return
            match = re.fullmatch(r"/api/actions/([^/]+)/log", path)
            if match:
                if not self._authorized():
                    self._send_json(HTTPStatus.FORBIDDEN, {"error": "forbidden"})
                    return
                try:
                    self._send_json(HTTPStatus.OK, {"output": service.action_log(match.group(1))})
                except ValueError as error:
                    self._send_json(HTTPStatus.BAD_REQUEST, {"error": str(error)})
                except KeyError as error:
                    self._send_json(HTTPStatus.NOT_FOUND, {"error": str(error)})
                return
            self._send_json(HTTPStatus.NOT_FOUND, {"error": "not found"})

        def do_POST(self) -> None:
            if not self._authorized():
                self._send_json(HTTPStatus.FORBIDDEN, {"error": "forbidden"})
                return
            path = urlparse(self.path).path
            try:
                payload = self._payload()
                if path == "/api/swaps":
                    result = service.start(
                        payload.get("seller"), payload.get("receive_address"),
                        payload.get("change_address"),
                    )
                else:
                    match = re.fullmatch(r"/api/swaps/([^/]+)/(resume|cancel)", path)
                    if not match:
                        self._send_json(HTTPStatus.NOT_FOUND, {"error": "not found"})
                        return
                    result = service.resume(match.group(1)) if match.group(2) == "resume" else service.cancel(match.group(1))
                self._send_json(HTTPStatus.ACCEPTED, result)
            except (ValueError, json.JSONDecodeError) as error:
                self._send_json(HTTPStatus.BAD_REQUEST, {"error": str(error)})
            except KeyError as error:
                self._send_json(HTTPStatus.NOT_FOUND, {"error": str(error)})

    return Handler
