#ifndef POLY1305_HPP
#define POLY1305_HPP

#include <cstdint>
#include <vector>
#include <array>
#include <cstring>

class Poly1305 {
public:
    // Generate Poly1305 key once (One-Time Key: 32 bytes r and s) from first ChaCha20 block
    static void computeTag(const uint8_t oneTimeKey[32], 
                           const std::vector<uint8_t>& ciphertext, 
                           std::array<uint8_t,16>& outTag) 
    {
        // Use RFC 8439 to clamp value r (16 start bytes of oneTimeKey )
        uint64_t r0 = load32_le(oneTimeKey + 0) & 0x0fffffff;
        uint64_t r1 = load32_le(oneTimeKey + 4) & 0x0ffffffc;
        uint64_t r2 = load32_le(oneTimeKey + 8) & 0x0ffffffc;
        uint64_t r3 = load32_le(oneTimeKey + 12) & 0x0ffffffc;

        // s (16 end bytes of oneTimeKey)
        uint32_t s0 = load32_le(oneTimeKey + 16);
        uint32_t s1 = load32_le(oneTimeKey + 20);
        uint32_t s2 = load32_le(oneTimeKey + 24);
        uint32_t s3 = load32_le(oneTimeKey + 28);

        // Accumulator h = 0
        uint64_t h0 = 0, h1 = 0, h2 = 0, h3 = 0, h4 = 0;

        size_t offset = 0;
        size_t len = ciphertext.size();

        while (len > 0) {
            size_t blockSize = (len >= 16) ? 16 : len;
            uint8_t block[17] = {0};
            std::memcpy(block, ciphertext.data() + offset, blockSize);
            block[blockSize] = 0x01; // add bytes 0x01 to the end of the block

            uint64_t b0 = load32_le(block + 0);
            uint64_t b1 = load32_le(block + 4);
            uint64_t b2 = load32_le(block + 8);
            uint64_t b3 = load32_le(block + 12);
            uint64_t b4 = block[16];

            h0 += b0;
            h1 += b1;
            h2 += b2;
            h3 += b3;
            h4 += b4;

            // Polynomial kernel h = (h * r) % (2^130 - 5)
            uint64_t d0 = h0 * r0 + h1 * (5 * r3) + h2 * (5 * r2) + h3 * (5 * r1);
            uint64_t d1 = h0 * r1 + h1 * r0       + h2 * (5 * r3) + h3 * (5 * r2);
            uint64_t d2 = h0 * r2 + h1 * r1       + h2 * r0       + h3 * (5 * r3);
            uint64_t d3 = h0 * r3 + h1 * r2       + h2 * r1       + h3 * r0;

            h0 = d0 & 0xFFFFFFFF;
            h1 = d1 + (d0 >> 32);
            h2 = d2 + (h1 >> 32);
            h3 = d3 + (h2 >> 32);
            h4 += (h3 >> 32);

            h1 &= 0xFFFFFFFF;
            h2 &= 0xFFFFFFFF;
            h3 &= 0xFFFFFFFF;

            offset += blockSize;
            len -= blockSize;
        }

        // Add s to the end value: (h + s) % 2^128
        uint64_t carry = 0;
        uint32_t final0 = static_cast<uint32_t>(h0 + s0);
        carry = (h0 + s0) >> 32;
        uint32_t final1 = static_cast<uint32_t>(h1 + s1 + carry);
        carry = (h1 + s1 + carry) >> 32;
        uint32_t final2 = static_cast<uint32_t>(h2 + s2 + carry);
        carry = (h2 + s2 + carry) >> 32;
        uint32_t final3 = static_cast<uint32_t>(h3 + s3 + carry);

        store32_le(outTag.data() + 0, final0);
        store32_le(outTag.data() + 4, final1);
        store32_le(outTag.data() + 8, final2);
        store32_le(outTag.data() + 12, final3);
    }

private:
    static inline uint32_t load32_le(const uint8_t* p) {
        return static_cast<uint32_t>(p[0]) |
              (static_cast<uint32_t>(p[1]) << 8) |
              (static_cast<uint32_t>(p[2]) << 16) |
              (static_cast<uint32_t>(p[3]) << 24);
    }

    static inline void store32_le(uint8_t* p, uint32_t val) {
        p[0] = static_cast<uint8_t>(val & 0xFF);
        p[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
        p[2] = static_cast<uint8_t>((val >> 16) & 0xFF);
        p[3] = static_cast<uint8_t>((val >> 24) & 0xFF);
    }
};

#endif // POLY1305_HPP