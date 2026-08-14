# Atomic Swaps as a Service bridge

This directory integrates the COMIT/UnstoppableSwap `swap` CLI with the Onion Monero Blockchain Explorer.
The explorer navigation exposes `/swap`, which redirects to an authenticated loopback sidecar.

Implemented bounty operations:

- start a BTC-to-XMR atomic swap (`buy-xmr`);
- show authoritative protocol history (`history`);
- resume by swap UUID (`resume`);
- protocol-safe cancel/refund by swap UUID (`cancel-and-refund`);
- keep a separate SQLite audit history of every bridge action.

## Compatible binary

Use the official `swap` 0.13.4 binary or a source build with the same CLI.
Newer Eigenwallet preview builds removed `buy-xmr`, so they are not compatible with this historical bounty interface.
The sidecar verifies that the configured path exists and is executable.

The expected command shapes are:

```text
swap buy-xmr --seller SELLER --receive-address XMR --change-address BTC
swap history
swap resume --swap-id UUID
swap cancel-and-refund --swap-id UUID
```

## Run

```bash
python3 tools/asaas/run.py \
  --swap-binary /absolute/path/to/swap \
  --data-dir "$HOME/.local/share/onion-explorer-asaas"
```

Open `http://127.0.0.1:8117/` and paste the token stored at:

```text
$HOME/.local/share/onion-explorer-asaas/api.token
```

The token file is created with mode `0600`.
The service binds only to loopback by default.
For a remote node, use an SSH tunnel:

```bash
ssh -L 8117:127.0.0.1:8117 operator@node
```

Do not expose the sidecar directly to the internet.
If remote binding is unavoidable, `--allow-remote` is explicit and the operator must provide TLS and access control in a reverse proxy.

## Test

```bash
cd tools/asaas
python3 -m unittest discover -s tests -v
```

The suite uses a fake executable and verifies command argument boundaries, injection rejection, UUID validation, start/history/resume/cancel flows, HTTP authorization, and UI serving.
It does not move mainnet funds.

## Security properties

- never invokes a shell;
- rejects control characters and malformed UUIDs before execution;
- requests `cancel-and-refund` instead of killing a swap process;
- stores command output in private local logs;
- protects all API data and mutations with a constant-time token check;
- adds no trackers, cookies, or third-party browser resources.
