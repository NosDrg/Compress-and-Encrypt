# Secure File Packaging Prototype

A C++17 command-line tool for compressing, encrypting, and restoring binary
files. The default pipeline uses Zstd compression followed by a cascade of
AES-256-GCM and ChaCha20-Poly1305 encryption. The result is stored in a
versioned binary packet with a nonce, authentication tag, file metadata, and a
CRC32 checksum.

> **Security status:** this is a prototype. The cascade currently stores one
> 16-byte tag created by XORing the AES-GCM and ChaCha20-Poly1305 tags. During
> decryption, the same stored tag is supplied to both layers. Review and test
> this design before relying on the format for confidential or adversarial
> environments. CRC32 is an additional corruption check, not a replacement for
> cryptographic authentication.

## Features

- Binary file input and output
- Zstd compression
- AES-256-GCM followed by ChaCha20-Poly1305 encryption
- A 32-byte master key split into independent AES and ChaCha20 subkeys
- Random 12-byte nonce stored in each packet
- 16-byte authentication tag stored in the packet header
- Packet header containing:
  - packet signature and format version
  - original and compressed sizes
  - original file extension
	- compression and encryption flags
	- ChaCha20 nonce
  - CRC32 checksum
- CRC32 validation after decryption and decompression
- Restoration of the original file contents after decoding

## Processing Flow

### Packaging

```text
input file
	-> read bytes
	-> Zstd compression
	-> AES-256-GCM encryption
	-> ChaCha20-Poly1305 encryption
	-> CRC32 checksum
	-> packet header + encrypted payload
	-> .sec file
```

### Unpackaging

```text
.sec file
	-> validate packet signature and version
	-> read packet header and encrypted payload
	-> ChaCha20-Poly1305 authentication and decryption
	-> AES-256-GCM authentication and decryption
	-> Zstd decompression
	-> verify CRC32
	-> restored file
```

## Build

The project requires a C++17 compiler, Zstandard (`libzstd` and `zstd.h`),
and OpenSSL (`libcrypto` and `openssl/evp.h`). Compile `main.cpp` and
`pack/packet.cpp`; the other project components are header-only.

### Windows with MinGW

```powershell
g++ -std=c++17 -O2 main.cpp pack/packet.cpp -lzstd -lcrypto -o secpack.exe
```

### Linux or macOS

```bash
g++ -std=c++17 -O2 main.cpp pack/packet.cpp -lzstd -lcrypto -o secpack
```

## Encryption Key

The program requires the `KEY_CRYPT` value before either packaging or
unpackaging a file. It must contain exactly 32 comma-separated hexadecimal
bytes. The value is read from `.env` first and then from the operating system
environment.

Create a local `.env` file (it is ignored by Git):

```dotenv
KEY_CRYPT=00,11,22,33,44,55,66,77,88,99,AA,BB,CC,DD,EE,FF,10,20,30,40,50,60,70,80,90,A0,B0,C0,D0,E0,F0,FF
```

For a one-off PowerShell command, set the environment variable instead:

```powershell
$env:KEY_CRYPT = "00,11,22,33,44,55,66,77,88,99,AA,BB,CC,DD,EE,FF,10,20,30,40,50,60,70,80,90,A0,B0,C0,D0,E0,F0,FF"
```

Keep the key private and use the same key when decoding the packet.

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
secpack -d <input-packet> [output-file]
```

Example:

```text
secpack -d image.sec image.jpg
```

The output path is optional. If it is omitted, the decoder creates
`<packet-base-name>_decompressed<stored-extension>` in the current directory.

The program returns exit code `0` on success and `1` when the key, input,
packet, or processing step is invalid.

## Packet Format

Each packet consists of a packed 63-byte header followed by the encrypted
payload. The header contains:

- `DRGO` signature and format version `1`
- flags (`0x03` is used by the CLI for compression and encryption)
- one padding byte reserved by the compressor interface
- 12-byte encryption nonce
- 16-byte authentication tag
- original file extension in an 8-byte field, including the terminator
- original file size and encrypted payload size as `uint64_t`
- CRC32 of the original restored data

The processing order is:

```text
compress -> encrypt -> package -> transmit
```

Decoding performs the reverse operations and checks the CRC32 after restoring
the original bytes.

## Project Layout

```text
.
├── main.cpp                    # CLI, key loading, and processing pipelines
├── compress/
	├── ZstdAdapter.hpp         # Zstd compression
	├── ICompressor.hpp
	├── bitReader.hpp
	└── bitWriter.hpp
├── crypto/
	├── AES256Adapter.hpp          # AES-256-GCM
	├── ChaCha20.hpp               # ChaCha20 implementation
	├── ChaCha20Adapter.hpp        # ChaCha20-Poly1305
	├── CascadeCipherAdapter.hpp
	├── ICipher.hpp
	└── cryptoEngine.hpp
└── pack/
	├── packet.cpp                 # Packet header serialization
	├── packet.hpp
	└── CRC32.hpp                  # Integrity checksum
```

## Limitations

- The cascade tag format needs security review because one XOR-combined tag is
	used for two independent AEAD layers.
- CRC32 is not a cryptographic integrity mechanism.
- The complete input file is loaded into memory.
- Empty input files are rejected.
- File extensions longer than 7 characters are truncated in the packet header.
- The packet format uses native little-endian integer representation and is not
	yet designed for cross-endian portability.

## Quick Round Trip

After building and configuring `KEY_CRYPT`, package and restore a sample file:

```powershell
Set-Content -Path sample.txt -Value "hello secure package"
.\secpack.exe -c sample.txt sample.sec
.\secpack.exe -d sample.sec restored.txt
Get-FileHash sample.txt
Get-FileHash restored.txt
```

The two hashes should be identical when encryption authentication succeeds.
