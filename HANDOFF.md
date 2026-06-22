# Cereblix Qt Wallet — Developer Handoff

Native **Qt 6 / C++** desktop wallet for Cereblix (CRB). Compatible with the official Go CLI wallet (`cereblix-wallet`) wallet file format and RPC API.

## Repository layout

| Path | Purpose |
|------|---------|
| `qt-wallet/` | Qt wallet source (CMake project) |
| `cereblix/` | Upstream Cereblix node + CLI wallet (clone/build separately) |
| `qt6/` | Local Qt 6.8.3 MSVC install (via aqtinstall; not committed) |
| `tools/deploy.ps1` | Copy Qt/libsodium/TLS plugins next to the `.exe` |
| `tools/crypto_validate.ps1` | Self-test + CLI wallet encryption smoke test |

## Build (Windows, MSVC)

**Prerequisites:** Visual Studio 2022, CMake 3.16+, vcpkg with `libsodium`, Qt 6.8+ (or use bundled `qt6/` from aqtinstall).

```powershell
cd qt-wallet
cmake -B build `
  -DCMAKE_PREFIX_PATH="..\qt6\6.8.3\msvc2022_64" `
  -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
powershell -File ..\tools\deploy.ps1
```

Run:

```powershell
.\build\Release\cereblix-qt-wallet.exe
.\build\Release\cereblix-qt-wallet.exe --self-test
```

### CLI options

| Flag | Description |
|------|-------------|
| `--self-test` | Crypto + network smoke test (no GUI) |
| `--unlock-wallet <path>` | Unlock wallet using `CEREBRA_PASSPHRASE` (test/CI) |
| `-n`, `--node <url>` | Override RPC URL |
| `-w`, `--wallet <path>` | Override wallet file path |

Environment: `CEREBRA_PASSPHRASE` — auto-unlock encrypted wallet on startup (visible to other processes; see [SECURITY.md](SECURITY.md)).

## Wallet file

- Default path: `%USERPROFILE%\.cereblix\wallet.json` (same as Go CLI)
- Format: JSON with ed25519 keys (`crb1…` addresses)
- Encryption: **PBKDF2-HMAC-SHA256** (200k iterations, RFC 2898) + AES-256-GCM (libsodium, requires AES-NI)
- Minimum passphrase: **12 characters** (Qt wallet; Go CLI allows 6)
- Linux: wallet file saved with mode `0600`
- Settings persisted via `QSettings` (registry on Windows): RPC URL, wallet path, local node options
- First address creation prompts for encryption (recommended)

## Consensus / signing

Chain ID is **embedded at compile time** in `qt-wallet/src/crypto/Consensus.h` and must match `core.ChainID` in the Go node:

```
74cedb08fc305c94fefe5976783b96236b857cd7cd94995b67e7ab4430f0a670
```

Signing height for chain-id activation: **700** (`ChainIdHeight`).

Transactions are signed with ed25519 over a deterministic payload (see `CereblixCrypto::signingPayload`). The wallet does **not** pass `chain_id` from RPC into signing — it uses the embedded constant so offline signing stays consistent.

**When forking:** update `Consensus.h` and rebuild whenever `core.ChainID` changes.

## RPC API (used endpoints)

| Endpoint | Use |
|----------|-----|
| `GET /status` | Network height, fees, peers, chain metadata |
| `GET /balance?addr=` | Balance, spendable, nonce |
| `GET /history?addr=&limit=` | Transaction history |
| `GET /tx?txid=` | Tx lookup (explorer, RBF) |
| `POST /tx` | Broadcast signed transaction |

Default public API: `https://cereblix.com/api`  
Alternate: `https://ru.cereblix.com/pool/api`

### TLS / deployment

HTTPS requires Qt TLS plugins beside the executable:

- `tls/qschannelbackend.dll` (Windows Schannel)
- `networkinformation/qnetworklistmanager.dll`
- `platforms/qwindows.dll`

Run `tools/deploy.ps1` after every build.

## Features (GUI tabs)

| Tab | Function |
|-----|----------|
| Overview | Total balance, network status, suggested fee |
| Receive | Address list + QR codes |
| Send | Build/sign/broadcast; auto fee; address picker |
| History | Per-address history; RBF speed-up / cancel |
| Addresses | Create, import, export keys; balance + spendable |
| Explorer | Tx and address lookup |
| Settings | RPC URL, wallet path, encrypt/lock, optional local `cereblixd` |

## Local node (optional)

Settings can spawn `cereblixd.exe` from:

1. Same directory as the wallet `.exe`
2. `../cereblix/cereblixd.exe` relative to the `.exe`

While syncing, the wallet uses the public API; when the local node catches up it switches RPC to `http://127.0.0.1:18751/api`.

## Parity vs Go CLI (`cereblix-wallet`)

| Feature | Qt wallet | Go CLI |
|---------|-----------|--------|
| Keygen / import | Yes | Yes |
| Encrypt / unlock | Yes | Yes |
| Send / fee | Yes (with confirmation dialog) | Yes |
| RBF speed-up / cancel | Yes | Yes |
| Balance / spendable | Yes | Yes |
| History | Yes | Yes |
| Tx lookup | Yes | Yes |
| Block explorer (blocks, richlist, search) | Partial (tx/addr only) | Full |
| Mempool CLI view | No | Yes |

## Testing

```powershell
.\build\Release\cereblix-qt-wallet.exe --self-test
powershell -File tools\crypto_validate.ps1
```

Self-test covers: ed25519 sign, PBKDF2-HMAC-SHA256 vector, wallet round-trip, encrypt/unlock, live `/status`.

`crypto_validate.ps1` verifies Qt can unlock a Go CLI–encrypted wallet (cross-KDF compatibility).

## Security

See **[SECURITY.md](SECURITY.md)** for threat model, encryption details, and operational risks.

**Note:** Wallets encrypted by Qt builds before the PBKDF2-HMAC fix cannot be opened by current builds. Re-encrypt from plaintext if needed.

## Known limitations

1. Explorer tab covers `/tx` and `/balance`+`/history` only — not full block explorer.
2. History RBF buttons appear on all rows; replacements only work for your own pending outgoing txs.
3. No installer/MSI yet — ship `build/Release/` folder after `deploy.ps1`.
4. Linux/macOS builds are possible with CMake + Qt6 + libsodium but not CI-tested in this repo.

## License

Qt wallet sources: MIT (see `LICENSE`). Third-party: Nayuki QR Code generator (MIT), libsodium (ISC), Qt (LGPL/commercial per your Qt license).

Upstream Cereblix node/wallet: follow licenses in `cereblix/`.
