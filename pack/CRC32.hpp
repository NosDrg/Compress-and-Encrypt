#ifndef CRC32_HPP
#define CRC32_HPP

#include <cstdint>
#include <vector>
#include <array>

struct CRC32 {
public:
    // Method to compute the CRC32 checksum for a given data buffer
    // This method uses a precomputed lookup table to efficiently calculate the CRC32 value.
    static constexpr std::array<uint32_t, 256> generateCRCTable() {
        std::array<uint32_t, 256> table{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            for (uint32_t j = 0; j < 8; ++j) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ 0xEDB88320;
                } else {
                    crc >>= 1;
                }
            }
            table[i] = crc;
        }
        return table;
    }

    // Method to compute the CRC32 checksum for a given data buffer
    static uint32_t compute(const std::vector<uint8_t>& data) {
        static const std::array<uint32_t, 256> crcTable = generateCRCTable();
        uint32_t crc = 0xFFFFFFFF;
        for (uint8_t byte : data) {
            crc = (crc >> 8) ^ crcTable[(crc ^ byte) & 0xFF];   // Update the CRC value using the lookup table
        }
        return ~crc ^ 0xFFFFFFFF; // Finalize the CRC value
    }
};


#endif // CRC32_HPP