#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <array>
#include <memory>
#include <cstdlib>
#include <algorithm>

#include "compress/ZstdAdapter.hpp"
#include "crypto/ChaCha20Adapter.hpp"
#include "pack/packet.hpp"
#include "pack/CRC32.hpp"

#include <fstream>
#include <sstream>

// Function to get config value: checks .env file first, then OS environment variables
std::string getEnvValue(const std::string& keyName, const std::string& envFilePath = ".env") {
    // 1. Try reading from the .env file
    std::ifstream envFile(envFilePath);
    if (envFile.is_open()) {
        std::string line;
        while (std::getline(envFile, line)) {
            // Trim whitespace
            size_t first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos || line[first] == '#') continue; // Skip comments and empty lines

            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string k = line.substr(0, eqPos);
                std::string v = line.substr(eqPos + 1);

                // Trim key
                size_t k_end = k.find_last_not_of(" \t\r\n");
                if (k_end != std::string::npos) k = k.substr(first, k_end - first + 1);

                if (k == keyName) {
                    // Trim value (strip optional quotes)
                    size_t v_start = v.find_first_not_of(" \t\r\n\"'");
                    size_t v_end = v.find_last_not_of(" \t\r\n\"'");
                    if (v_start != std::string::npos && v_end != std::string::npos) {
                        return v.substr(v_start, v_end - v_start + 1);
                    }
                    return "";
                }
            }
        }
    }

    // 2. Fallback to OS environment variable
    const char* os_val = std::getenv(keyName.c_str());
    return os_val ? std::string(os_val) : "";
}

// Load 32-byte secret key from .env file or environment variable KEY_CRYPT
bool loadKeyFromEnv(std::array<uint8_t, 32>& outKey) {
    std::string env_val = getEnvValue("KEY_CRYPT");
    if (env_val.empty()) {
        std::cerr << "Error: 'KEY_CRYPT' not found in .env file or environment variables." << std::endl;
        return false;
    }

    std::stringstream ss(env_val);
    std::string token;
    std::vector<uint8_t> parsed_bytes;

    while (std::getline(ss, token, ',')) {
        size_t start = token.find_first_not_of(" \t\r\n");
        size_t end = token.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        token = token.substr(start, end - start + 1);

        try {
            unsigned long val = std::stoul(token, nullptr, 16);
            if (val > 0xFF) {
                std::cerr << "The value " << token << " is not a valid byte (must be between 0x00 and 0xFF)\n";
                return false;
            }
            parsed_bytes.push_back(static_cast<uint8_t>(val));
        } catch (const std::exception&) {
            std::cerr << "Invalid hex string: " << token << "\n";
            return false;
        }
    }

    if (parsed_bytes.size() != 32) {
        std::cerr << "The key must be exactly 32 bytes (currently: " 
                  << parsed_bytes.size() << " bytes)!\n";
        return false;
    }

    std::copy(parsed_bytes.begin(), parsed_bytes.end(), outKey.begin());
    return true;
}

// Function to read a file into a vector of bytes
bool readFileToVector(const std::string& filePath, std::vector<uint8_t>& data) {
    std::ifstream inputFile(filePath, std::ios::binary);
    if (!inputFile) {
        std::cerr << "Error opening file for reading: " << filePath << std::endl;
        return false;
    }

    // Read the entire file into the vector
    data.assign(std::istreambuf_iterator<char>(inputFile), std::istreambuf_iterator<char>());
    return true;
}

// Function to write a vector of bytes to a file
std::string getFileExtension(const std::string& filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return ""; // No extension found
    }
    return filePath.substr(dotPos); // Return the extension including the dot
}

// Extract base file name without path and extension
std::string getBaseFileName(const std::string& filePath) {
    size_t lastSlash = filePath.find_last_of("/\\");
    std::string fileName = (lastSlash == std::string::npos) ? filePath : filePath.substr(lastSlash + 1);
    size_t dotPos = fileName.find_last_of('.');
    return (dotPos == std::string::npos) ? fileName : fileName.substr(0, dotPos);
}

// ==========================================
// Modular Step Functions
// ==========================================

// Step 1: Compress raw data into compressed byte stream
bool compressDataStep(ICompressor* compressor, const std::vector<uint8_t>& rawData, 
                      std::vector<uint8_t>& compressedData, uint8_t& padding) {
    return compressor->compress(rawData, compressedData, padding);
}

// Step 2: Encrypt compressed payload using ChaCha20 stream cipher
bool encryptPayloadStep(ICipher* cipher, const std::vector<uint8_t>& compressedData, 
                        const std::array<uint8_t, 32>& key, std::array<uint8_t, 12>& nonce, 
                        std::vector<uint8_t>& encryptedData) {
    cipher->generateNonce(nonce);
    return cipher->encrypt(compressedData, key, nonce, encryptedData);
}

// Step 3: Decrypt encrypted payload back to compressed data
bool decryptPayloadStep(ICipher* cipher, const std::vector<uint8_t>& encryptedData, 
                        const std::array<uint8_t, 32>& key, std::array<uint8_t, 12>& nonce, 
                        std::vector<uint8_t>& decryptedData) {
    return cipher->decrypt(encryptedData, key, nonce, decryptedData);
}

// Step 4: Decompress data back to original content
bool decompressDataStep(ICompressor* compressor, const std::vector<uint8_t>& decryptedData, 
                        uint64_t originalSize, std::vector<uint8_t>& restoredData) {
    return compressor->decompress(decryptedData, originalSize, restoredData);
}

// ================================
// Pipeline packaging
// ================================

bool compressAndPackageFile(const std::string& inputFilePath, const std::string& outputFilePath, 
                            const std::array<uint8_t, 32>& key, ICompressor* compressor, ICipher* cipher) {
    // Read the input file into a vector of bytes
    std::vector<uint8_t> inputData;
    if (!readFileToVector(inputFilePath, inputData)) {
        return false; // Error reading the file
    }

    uint64_t originalSize = inputData.size();
    if (originalSize == 0) {
        std::cerr << "Input file is empty: " << inputFilePath << std::endl;
        return false; // Empty file
    }

    // Compute CRC32 checksum on original raw data for end-to-end integrity verification
    uint32_t originalCrc32 = CRC32::compute(inputData);

    // Compress the data using Huffman coding (Compression must precede encryption)
    std::vector<uint8_t> compressedData;
    uint8_t remainingBits = 0;
    if (!compressDataStep(compressor, inputData, compressedData, remainingBits)) {
        std::cerr << "Compression failed for file: " << inputFilePath << std::endl;
        return false;
    }

    // Encrypt the compressed data using ChaCha20
    std::array<uint8_t, 12> nonce;
    std::vector<uint8_t> encryptedPayload;
    if (!encryptPayloadStep(cipher, compressedData, key, nonce, encryptedPayload)) {
        std::cerr << "Encryption failed for file: " << inputFilePath << std::endl;
        return false;
    }

    uint64_t payloadSize = encryptedPayload.size();

    // Create the packet header
    std::string fileExtension = getFileExtension(inputFilePath);
    PacketHeader header = PacketManager::createPacketHeader(
        fileExtension,
        originalSize,
        payloadSize,
        remainingBits,
        originalCrc32,
        0x03, // Flags: 0x01 (Huffman) | 0x02 (ChaCha20)
        nonce.data()
    );

    // Write the packet header and compressed data to the output file
    std::ofstream outputFile(outputFilePath, std::ios::binary);
    if (!outputFile) {
        std::cerr << "Error opening output file for writing: " << outputFilePath << std::endl;
        return false; // Error opening the output file
    }

    PacketManager::writeHeader(outputFile, header);
    outputFile.write(reinterpret_cast<const char*>(encryptedPayload.data()), 
                     static_cast<std::streamsize>(payloadSize));
    outputFile.close();

    double compressionRatio = static_cast<double>(payloadSize) / static_cast<double>(originalSize);
    std::cout << "File compressed and packaged successfully!" << std::endl;
    std::cout << "Original Size: " << originalSize << " bytes" << std::endl;
    std::cout << "Compressed Size: " << payloadSize << " bytes" << std::endl;
    std::cout << "Compression Ratio: " << std::fixed << std::setprecision(2) << (compressionRatio * 100) << "%" << std::endl;
    std::cout << "CRC32 Checksum: 0x" << std::hex << std::uppercase << originalCrc32 << std::dec << std::endl;
    std::cout << "Saved to: " << outputFilePath << std::endl;

    return true;
}

// ================================
// Pipeline unpackaging
// ================================

bool decompressAndUnpackageFile(const std::string& inputFilePath, std::string outputFilePath, 
                                const std::array<uint8_t, 32>& key, ICompressor* compressor, ICipher* cipher) {
    // Open the input file for reading
    std::ifstream inputFile(inputFilePath, std::ios::binary);
    if (!inputFile) {
        std::cerr << "Error opening input file for reading: " << inputFilePath << std::endl;
        return false; // Error opening the input file
    }

    // Read the packet header from the input file
    PacketHeader header;
    if (!PacketManager::readHeader(inputFile, header)) {
        std::cerr << "Error reading packet header from file: " << inputFilePath << std::endl;
        return false; // Error reading the header
    }

    // Auto-generate destination file name if not provided
    if (outputFilePath.empty()) {
        outputFilePath = getBaseFileName(inputFilePath) + "_decompressed" + std::string(header.file_ext);
    }

    // Read the encrypted payload from the input file
    std::vector<uint8_t> encryptedPayload(header.payload_size);
    inputFile.read(reinterpret_cast<char*>(encryptedPayload.data()), header.payload_size);
    if (inputFile.gcount() != static_cast<std::streamsize>(header.payload_size)) {
        std::cerr << "Error reading compressed data from file: " << inputFilePath << std::endl;
        return false; // Error reading the compressed data
    }

    // Decrypt the payload using ChaCha20
    std::vector<uint8_t> compressedData;
    std::array<uint8_t, 12> nonce;
    std::copy(std::begin(header.nonce), std::end(header.nonce), nonce.begin());
    if (!decryptPayloadStep(cipher, encryptedPayload, key, nonce, compressedData)) {
        std::cerr << "Decryption failed for file: " << inputFilePath << std::endl;
        return false;
    }

    // Decompress the data using Huffman coding
    std::vector<uint8_t> restoredData;
    if (!decompressDataStep(compressor, compressedData, header.original_size, restoredData)) {
        std::cerr << "Decompression failed for file: " << inputFilePath << std::endl;
        return false;
    }

    // Verify CRC32 checksum of the decompressed data
    uint32_t computedCRC32 = CRC32::compute(restoredData);
    if (computedCRC32 != header.crc32) {
        std::cerr << "CRC32 checksum mismatch! Data may be corrupted." << std::endl;
        return false; // Checksum mismatch
    }

    // Write the decompressed data to the output file
    std::ofstream outputFile(outputFilePath, std::ios::binary);
    if (!outputFile) {
        std::cerr << "Error opening output file for writing: " << outputFilePath << std::endl;
        return false; // Error opening the output file
    }

    outputFile.write(reinterpret_cast<const char*>(restoredData.data()),
                     static_cast<std::streamsize>(restoredData.size()));
    outputFile.close();

    std::cout << "File decompressed and unpackaged successfully!" << std::endl;
    std::cout << "Decompressed Size: " << restoredData.size() << " bytes" << std::endl;
    std::cout << "Saved to: " << outputFilePath << std::endl;
    std::cout << "CRC32 Checksum Verified: 0x" << std::hex << std::uppercase << computedCRC32 << std::dec << std::endl;

    return true;
}

// ===============================
// CLI Interface
// ===============================

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <mode> <input_file> [output_file]" << std::endl;
    std::cout << "  Mode:" << std::endl;
    std::cout << "    -c: Compress and package a file" << std::endl;
    std::cout << "    -d: Decompress and unpackage a file" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1; // Invalid number of arguments
    }

    std::string mode = argv[1];

    // Load encryption key from environment variable
    std::array<uint8_t, 32> secretKey;
    if (!loadKeyFromEnv(secretKey)) {
        return 1;
    }

    // Instantiate compression and encryption adapters via abstraction interfaces
    std::unique_ptr<ICompressor> huffmanCompressor = std::make_unique<ZstdAdapter>();
    std::unique_ptr<ICipher> chacha20Cipher = std::make_unique<ChaCha20Adapter>();

    if (mode == "-c") {
        if (argc < 4) {
            std::cerr << "Error: Missing input or output file argument." << std::endl;
            printUsage(argv[0]);
            return 1; // Invalid number of arguments for the specified mode
        }
        
        return compressAndPackageFile(argv[2], argv[3], secretKey, huffmanCompressor.get(), chacha20Cipher.get()) ? 0 : 1; // Compress and package
    } else if (mode == "-d") {
        // Output file argument is optional; empty string triggers auto-generation
        std::string outputFilePath = (argc >= 4) ? argv[3] : "";

        // Decompress and unpackage the file
        if (!decompressAndUnpackageFile(argv[2], outputFilePath, secretKey, huffmanCompressor.get(), chacha20Cipher.get())) {
            std::cerr << "Decompression and unpackaging failed." << std::endl;
            return 1; // Error during decompression
        }
    } else {
        std::cerr << "Error: Invalid mode specified. Use -c for compress or -d for decompress." << std::endl;
        printUsage(argv[0]);
        return 1; // Invalid mode specified
    }

    return 0; // Success
}