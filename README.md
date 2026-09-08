# Secure File Packaging Prototype

A small C++ command-line project for preparing files for transmission. The
current implementation compresses a file with Huffman coding, stores it in a
versioned binary packet, and verifies the payload with CRC32 after transfer.

> **Security status:** encryption is not implemented yet. Compression reduces
> size, and CRC32 detects accidental corruption, but neither one protects the
> file from being read or modified by an attacker. Do not use the current
> format for confidential data until authenticated encryption is added.

## Features

- Binary file input and output
- Huffman compression
- Packet header containing:
  - packet signature and format version
  - original and compressed sizes
  - original file extension
  - compression flags
  - CRC32 checksum
- CRC32 validation before decompression
- Restoration of the original file contents after decoding

## Processing Flow

### Packaging

```text
input file
	-> read bytes
	-> Huffman compression
	-> CRC32 checksum
	-> packet header + compressed payload
	-> .sec file
```

### Unpackaging

```text
.sec file
	-> validate packet signature and version
	-> read packet header and payload
	-> verify CRC32
	-> Huffman decompression
	-> restored file
```

## Build

The project uses standard C++17 library features and has no third-party
dependencies. Build all `.cpp` files with a C++17 compiler:

### Windows with MinGW

```powershell
g++ -std=c++17 -O2 main.cpp compress/compressedData.cpp pack/packet.cpp -o secpack.exe
```

### Linux or macOS

```bash
g++ -std=c++17 -O2 main.cpp compress/compressedData.cpp pack/packet.cpp -o secpack
```

## Usage

Compress and package a file:

```text
secpack -c <input-file> <output-packet>
```

Example:

```text
secpack -c image.jpg image.sec
```

Decompress and restore a packet:

```text
secpack -d <input-packet> <output-file>
```

Example:

```text
secpack -d image.sec image.jpg
```

The decoder currently derives the output filename from the packet filename and
stored extension. The third argument is required by the command-line interface
but is not used to select the output path yet.

## Packet Format

Each packet consists of a fixed-size header followed by the compressed
payload. The header includes a `DRGO` signature, version `1`, file extension,
original size, payload size, CRC32 checksum, and flags. Flag bit 0 identifies
Huffman compression. The header also reserves flag bit 1 for encryption, but
the current code does not implement encryption.

## Planned Encryption

For safe transmission of sensitive files, the next version should encrypt the
compressed payload with an authenticated encryption algorithm such as
AES-256-GCM or ChaCha20-Poly1305. The packet should then include a nonce and
authentication tag, and decoding should authenticate the ciphertext before
decompression. A password should be converted to a key with a dedicated KDF
such as Argon2id or scrypt; a plain password must never be used directly as a
key.

The recommended order is:

```text
compress -> encrypt and authenticate -> package -> transmit
```

and the reverse order for decoding.

## Project Layout

```text
.
├── main.cpp                    # Command-line interface and pipelines
├── compress/
│   ├── compressedData.cpp      # Huffman encoder and decoder
│   ├── compressedData.hpp
│   └── data/                   # Bit-level stream helpers
└── pack/
	├── packet.cpp              # Packet header serialization
	├── packet.hpp
	└── CRC32.hpp               # Payload integrity checksum
```

## Limitations

- Encryption and authentication are not implemented.
- CRC32 is not a cryptographic integrity mechanism.
- The complete input file is loaded into memory.
- Empty input files are rejected.
- The decoder does not yet use the requested output path.
