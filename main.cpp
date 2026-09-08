#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

#include "compress/compressedData.hpp"
#include "pack/packet.hpp"
#include "pack/CRC32.hpp"

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

// ================================
// Pipeline packaging
// ================================

bool compressAndPackageFile(const std::string& inputFilePath, const std::string& outputFilePath) {
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

    // Compress the data using Huffman coding
    std::stringstream compressedStream(std::ios::in | std::ios::out | std::ios::binary);
    uint8_t remainingBits = CompressedDataHandler::compressData(inputData, compressedStream);

    // Convert the compressed data from the stringstream to a vector of bytes
    std::string compressedDataStr = compressedStream.str();
    uint64_t compressedSize = compressedDataStr.size();
    std::vector<uint8_t> compressedData(compressedDataStr.begin(), compressedDataStr.end());

    // Calculate CRC32 checksum of the compressed data
    uint32_t crc32Checksum = CRC32::compute(compressedData);

    // Create the packet header
    std::string fileExtension = getFileExtension(inputFilePath);
    PacketHeader header = PacketManager::createPacketHeader(
        fileExtension,
        originalSize,
        compressedSize,
        remainingBits,
        crc32Checksum,
        0x01 // Flags indicating Huffman compression
    );

    // Write the packet header and compressed data to the output file
    std::ofstream outputFile(outputFilePath, std::ios::binary);
    if (!outputFile) {
        std::cerr << "Error opening output file for writing: " << outputFilePath << std::endl;
        return false; // Error opening the output file
    }

    PacketManager::writeHeader(outputFile, header);
    outputFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedSize);

    outputFile.close();

    double compressionRatio = static_cast<double>(compressedSize) / static_cast<double>(originalSize);
    std::cout << "File compressed and packaged successfully!" << std::endl;
    std::cout << "Original Size: " << originalSize << " bytes" << std::endl;
    std::cout << "Compressed Size: " << compressedSize << " bytes" << std::endl;
    std::cout << "Compression Ratio: " << std::fixed << std::setprecision(2) << (compressionRatio * 100) << "%" << std::endl;
    std::cout << "CRC32 Checksum: 0x" << std::hex << std::uppercase << crc32Checksum << std::dec << std::endl;
    std::cout << "Saved to: " << outputFilePath << std::endl;

    return true;
}

// ================================
// Pipeline unpackaging
// ================================

bool decompressAndUnpackageFile(const std::string& inputFilePath) {
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

    // Read the compressed data from the input file
    std::vector<uint8_t> compressedData(header.payload_size);
    inputFile.read(reinterpret_cast<char*>(compressedData.data()), header.payload_size);
    if (inputFile.gcount() != static_cast<std::streamsize>(header.payload_size)) {
        std::cerr << "Error reading compressed data from file: " << inputFilePath << std::endl;
        return false; // Error reading the compressed data
    }

    // Verify CRC32 checksum of the compressed data
    uint32_t computedCRC32 = CRC32::compute(compressedData);
    if (computedCRC32 != header.crc32) {
        std::cerr << "CRC32 checksum mismatch! Data may be corrupted." << std::endl;
        return false; // Checksum mismatch
    }

    // Decompress the data using Huffman coding
    std::stringstream compressedStream(std::ios::in | std::ios::out | std::ios::binary);
    compressedStream.write(reinterpret_cast<const char*>(compressedData.data()), header.payload_size);
    compressedStream.seekg(0); // Reset stream position for reading

    std::vector<uint8_t> decompressedData = CompressedDataHandler::decompressData(compressedStream, header.original_size);

    // Write the decompressed data to the output file
    std::string outputFilePath = getFileExtension(inputFilePath) + "_decompressed" + std::string(header.file_ext);
    std::ofstream outputFile(outputFilePath, std::ios::binary);
    if (!outputFile) {
        std::cerr << "Error opening output file for writing: " << outputFilePath << std::endl;
        return false; // Error opening the output file
    }

    outputFile.write(reinterpret_cast<const char*>(decompressedData.data()),
                     static_cast<std::streamsize>(decompressedData.size()));
    outputFile.close();

    std::cout << "File decompressed and unpackaged successfully!" << std::endl;
    std::cout << "Decompressed Size: " << decompressedData.size() << "bytes" << std::endl;
    std::cout << "Saved to: " << outputFilePath << std::endl;
    std::cout << "CRC32 Checksum Verified: 0x" << std::hex << std::uppercase << computedCRC32 << std::dec << std::endl;

    return true;
}

// ===============================
// CLI Interface
// ===============================

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <mode> <input_file> <output_file>" << std::endl;
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

    if (mode == "-c") {
        if (argc < 4) {
            std::cerr << "Error: Missing input or output file argument." << std::endl;
            printUsage(argv[0]);
            return 1; // Invalid number of arguments for the specified mode
        }
        
        return compressAndPackageFile(argv[2], argv[3]) ? 0 : 1; // Compress and package
    } else if (mode == "-d") {
        if (argc < 4) {
            std::cerr << "Error: Missing input or output file argument." << std::endl;
            printUsage(argv[0]);
            return 1; // Invalid number of arguments for the specified mode
        }
        // Decompress and unpackage the file
        if (!decompressAndUnpackageFile(argv[2])) {
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