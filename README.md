# Secure File Packaging Prototype

A small C++17 command-line tool for preparing files for transmission. The
program compresses the input with Huffman coding, encrypts the compressed
payload with ChaCha20, stores it in a versioned binary packet, and verifies the
restored data with CRC32.

> **Security status:** the project currently uses ChaCha20 without an
> authentication tag. CRC32 detects some accidental corruption, but it is not
> cryptographic authentication and does not prevent intentional tampering. Do
> not rely on this format for confidential or adversarial environments.

## Features

- Binary file input and output
- Huffman compression
- ChaCha20 encryption with a 32-byte key
- Random 12-byte nonce stored in each packet
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
	-> Huffman compression
	-> ChaCha20 encryption
	-> CRC32 checksum
	-> packet header + encrypted payload
	-> .sec file
```

### Unpackaging

```text
.sec file
	-> validate packet signature and version
	-> read packet header and encrypted payload
	-> ChaCha20 decryption
	-> Huffman decompression
	-> verify CRC32
	-> restored file
```

## Build

The project uses standard C++17 library features and has no third-party
dependencies. Compile `main.cpp`, `compress/Huffman.cpp`, and
`pack/packet.cpp` with a C++17 compiler. The remaining components are
header-only.

### Windows with MinGW

```powershell
g++ -std=c++17 -O2 main.cpp compress/Huffman.cpp pack/packet.cpp -o secpack.exe
```

### Linux or macOS

```bash
g++ -std=c++17 -O2 main.cpp compress/Huffman.cpp pack/packet.cpp -o secpack
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

The output path is optional. If it is omitted, the decoder creates a file named
`<packet-base-name>_decompressed<stored-extension>` in the current directory.

The program returns exit code `0` on success and `1` when the key, input,
packet, or processing step is invalid.

## Packet Format

Each packet consists of a packed 47-byte header followed by the encrypted
payload. The header contains:

- `DRGO` signature and format version `1`
- flags: bit 0 for Huffman compression and bit 1 for ChaCha20 encryption
- Huffman padding count
- 12-byte ChaCha20 nonce
- original file extension (up to 7 characters)
- original file size and encrypted payload size
- CRC32 of the original, restored data

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
	├── Huffman.cpp                # Huffman encoder and decoder
	├── Huffman.hpp
	├── HuffmanAdapter.hpp
	├── ICompressor.hpp
	├── bitReader.hpp
	└── bitWriter.hpp
├── crypto/
	├── ChaCha20.hpp               # ChaCha20 implementation
	├── ChaCha20Adapter.hpp
	├── ICipher.hpp
	└── cryptoEngine.hpp
└── pack/
	├── packet.cpp                 # Packet header serialization
	├── packet.hpp
	└── CRC32.hpp                  # Integrity checksum
```

## Limitations

- ChaCha20 encryption is implemented, but authentication is not.
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

The two hashes should be identical.
