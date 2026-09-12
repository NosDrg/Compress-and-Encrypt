#ifndef ICIPHER_HPP
#define ICIPHER_HPP

#include <vector>
#include <cstdint>

// Interface for a cipher that defines methods for encryption and decryption
class ICipher {
public:
    virtual ~ICipher() = default;
    virtual void generateNonce(std::array<uint8_t, 12>& nonce) = 0;
    virtual bool encrypt(const std::vector<uint8_t>& input,
                         const std::array<uint8_t, 32>& key,
                         const std::array<uint8_t, 12>& nonce,
                         std::vector<uint8_t>& output,
                         std::array<uint8_t, 16>& outTag) = 0;
    virtual bool decrypt(const std::vector<uint8_t>& input,
                         const std::array<uint8_t, 32>& key,
                         const std::array<uint8_t, 12>& nonce,
                         const std::array<uint8_t, 16>& expectedTag,
                         std::vector<uint8_t>& output) = 0;
};

#endif // ICIPHER_HPP