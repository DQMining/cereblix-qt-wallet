# Cereblix Qt Wallet

Native Qt 6 desktop wallet for [Cereblix (CRB)](https://cereblix.com/).

## Features

- Create and manage `crb1…` addresses (compatible with the CLI wallet)
- Send and receive CRB via the public API or a local node
- Transaction history, RBF speed-up/cancel, block explorer (tx/address lookup)
- QR codes on Receive tab, wallet encryption (PBKDF2 + AES-GCM)
- Optional local `cereblixd` node with sync progress

## Prerequisites

- **Qt 6** (Core, Widgets, Network)
- **CMake 3.16+**
- **C++17 compiler** (MSVC 2019+, MinGW, or GCC)

## Build (Windows)

```powershell
# Install Qt 6 from https://www.qt.io/download-qt-installer
# Then configure (adjust Qt path):
cd "C:\CRB QT Wallet\qt-wallet"
cmake -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
cmake --build build --config Release
```

With vcpkg:

```powershell
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release

# Deploy Qt + libsodium DLLs next to the executable
powershell -File ..\tools\deploy.ps1
```

`deploy.ps1` copies TLS plugins required for HTTPS (`tls\qschannelbackend.dll`). Without them the wallet cannot reach `https://cereblix.com/api`.

The wallet auto-refreshes balances every 60 seconds. Use **File → Refresh** for an immediate update.

Run:

```powershell
.\build\Release\cereblix-qt-wallet.exe
.\build\Release\cereblix-qt-wallet.exe --self-test
```

Validate against the CLI wallet:

```powershell
powershell -File ..\tools\crypto_validate.ps1
```

## Wallet file

Uses the same path as the CLI wallet: `%USERPROFILE%\.cereblix\wallet.json`

## Developer handoff

See **[HANDOFF.md](HANDOFF.md)** for build details, consensus constants, RPC parity vs the Go CLI, and deployment notes for coin developers.

## Reference

The upstream Go CLI is in [`cereblix/`](cereblix/). Build it with:

```powershell
cd cereblix
go build -o cereblix-wallet.exe ./cmd/cereblix-wallet
```

## License

MIT (wallet code). Cereblix upstream is MIT.
