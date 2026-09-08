#include "packet.hpp"
#include <cstring>

const char PacketManager::signature[4] = {'D', 'R', 'G', 'O'};

PacketHeader PacketManager::createPacketHeader(
    const std::string& fileExtension,
    uint64_t originalSize,
    uint64_t payloadSize, 
    uint8_t padding,
    uint32_t crc32, 
    uint8_t flags) 
{
    PacketHeader header;
    std::memcpy(header.signature, signature, sizeof(signature));
    header.version = 0x01;
    header.flags = flags;
    header.padding = padding;

    // Copy the file extension into the header, ensuring it is null-terminated
    std::memset(header.file_ext, 0, sizeof(header.file_ext)); // Initialize with null characters
    std::strncpy(header.file_ext, fileExtension.c_str(), sizeof(header.file_ext) - 1);
    header.file_ext[sizeof(header.file_ext) - 1] = '\0'; // Ensure null-termination

    header.original_size = originalSize;
    header.payload_size = payloadSize;
    header.crc32 = crc32;

    return header;
}

bool PacketManager::writeHeader(std::ostream& outputStream, const PacketHeader& header) {
    outputStream.write(reinterpret_cast<const char*>(&header), sizeof(PacketHeader));
    return outputStream.good(); // Return true if the write operation was successful
}

bool PacketManager::readHeader(std::istream& inputStream, PacketHeader& header) {
    inputStream.read(reinterpret_cast<char*>(&header), sizeof(PacketHeader));
    if (std::memcmp(header.signature, signature, sizeof(signature)) != 0) {
        // Signature does not match, indicating an invalid packet format
        std::cerr << "Invalid packet signature: " << std::string(header.signature, sizeof(header.signature)) << std::endl;
        return false;
    }

    if (header.version != 0x01) {
        // Version mismatch, indicating an unsupported packet format
        std::cerr << "Unsupported packet version: " << static_cast<int>(header.version) << std::endl;
        return false;
    }

    return inputStream.good(); // Return true if the read operation was successful
}

