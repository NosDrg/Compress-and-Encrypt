#ifndef PACKET_HPP
#define PACKET_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

#pragma pack(push, 1) // Ensure no padding is added to the struct
// Structure to represent the header of a packet
struct PacketHeader {
    char signature[4];      // Signature to identify the packet format (e.g., "PACK")
    uint8_t version;        // Version of the packet format
    uint8_t flags;          // Bit 0: Hufman (1=0n), Bit 1: Encrypted (0=None, 1=AES)
    uint8_t padding;        // Padding byte for alignment
    uint8_t nonce[12];      // Nonce for encryption
    char file_ext[8];       // File extension e.g., ".txt", ".jpg" (end by '\0' if shorter than 8 characters)
    uint64_t original_size; // Original size of the file before compression
    uint64_t payload_size;  // Size of the payload (compressed data) in bytes
    uint32_t crc32;         // CRC32 checksum of the payload for integrity verification
};
#pragma pack(pop) // Restore the previous packing alignment


// Class to manage packet creation, reading, and writing
class PacketManager {
public:
    static const char signature[4];

    static PacketHeader createPacketHeader(
        const std::string& fileExtension,
         uint64_t originalSize,
         uint64_t payloadSize, 
         uint8_t padding,
         uint32_t crc32, 
         uint8_t flags = 0x01,
         const uint8_t nonce[12] = nullptr // Default to no nonce
    );

    // Method to write the packet header to an output stream
    static bool writeHeader(std::ostream& outputStream, const PacketHeader& header);

    // Method to read the packet header from an input stream
    static bool readHeader(std::istream& inputStream, PacketHeader& header);
};

#endif // PACKET_HPP