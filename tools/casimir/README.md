# Casimir

## Introduction

Casimir aims to provide virtual tag discovery and emulation for NFC
applications, in order to unlock NFC capability in various testing
contexes.

Casimir includes a virtual NFCC implementation, and internally emulates
RF communications between multiple connected hosts.

## Usage

```
$ cargo run -- --help
Usage: casimir [--nci-port <nci-port>]

Nfc emulator.

Options:
  --nci-port        configure the TCP port for the NCI server.
  --help            display usage information
```
