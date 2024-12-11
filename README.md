# BTC Mining Pool 

---

## Features

- **Task Generator**: Dynamically generates mining tasks based on blockchain state (e.g., block height, new transactions).
- **Block Generator**: Simulates block creation and validation.
- **Node Communication**: Fetches blockchain data using RPC from services like [GetBlock](https://getblock.io).

---

## Getting Started

Ensure the following tools and libraries are installed:
- **Compiler**: GCC or Clang with C++14 support.
- **Libraries**:
  - [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
  - [libcurl](https://curl.se/libcurl/)
  - [OpenSSL](https://www.openssl.org/)

Install these libraries on macOS using Homebrew:
```bash
brew install jsoncpp curl openssl
```

Compile:
```bash
make all
```

Run:
```bash
make run
```
