# Security

## Threat model

The Cereblix Qt wallet is a **non-custodial** desktop client:

- Private keys are generated and stored on your machine.
- Transactions are signed locally; only the signed transaction JSON is sent to the node RPC.
- The wallet does not upload seeds, passphrases, or private keys to any server.

Default RPC endpoints:

- `https://cereblix.com/api`
- `https://ru.cereblix.com/pool/api` (preset)
- `http://127.0.0.1:18751/api` (optional local `cereblixd`)

Custom RPC URLs are allowed for self-hosted nodes. The wallet warns when you connect to a non-default host.

## Wallet file encryption

- **KDF:** PBKDF2-HMAC-SHA256 (RFC 2898), 200,000 iterations — compatible with the Go `cereblix-wallet` CLI.
- **Cipher:** AES-256-GCM (requires CPU AES-NI / hardware AES support).
- **Minimum passphrase:** 12 characters; cannot be all digits.
- **Default:** New wallets are **not** encrypted until you set a passphrase. You are prompted to encrypt when creating your first address.

Wallets encrypted by builds **before** the PBKDF2 fix (pre-2026-06) used a non-standard KDF and cannot be opened by current builds or the Go CLI. Re-encrypt from a plaintext backup if you have one.

## Operational risks

| Risk | Mitigation |
|------|------------|
| Plaintext `wallet.json` | Encrypt the wallet; restrict file permissions (`0600` on Linux). |
| Weak passphrase | Use 12+ characters; avoid dictionary words. |
| Clipboard sniffing | Private key export shows on-screen; clipboard auto-clears after 30s. |
| `CEREBRA_PASSPHRASE` env var | Visible to other processes on the same user account; use only for automation/CI. |
| Wrong send address | Send requires a confirmation dialog before broadcast. |
| Malicious RPC node | Could show fake balances or censor txs; use official or self-hosted nodes you trust. |

## Memory

Passphrases and decrypted key material are cleared from memory on wallet lock where possible (`sodium_memzero`). OS swap and crash dumps are outside the wallet’s control.

## Reporting vulnerabilities

If you discover a security issue in this wallet, report it privately to the Cereblix maintainers via their official contact channel (GitHub security advisory or project email). Do not disclose exploit details publicly before a fix is available.

## What we do not claim

- Protection against malware on your PC with keyloggers or screen capture.
- Protection if you paste a wrong address and confirm the send dialog.
- Resistance to physical access without encryption.
