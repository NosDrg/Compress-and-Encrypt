#ifndef CRYPTO_ENGINE_HPP
#define CRYPTO_ENGINE_HPP

#include "ChaCha20.hpp"
#include <vector>
#include <cstdint>
#include <random>

class CryptoEngine {
public:
    static void generateNonce(std::array<uint8_t, 12>& nonce) {
        std::random_device rd;
        for (int i = 0; i < 12; ++i) {
            nonce[i] = static_cast<uint8_t>(rd() & 0xFF);
        }
    }

    static bool encrypt(const std::vector<uint8_t>& plaintext,
                        const std::array<uint8_t, 32>& key,
                        const std::array<uint8_t, 12>& nonce,
                        std::vector<uint8_t>& ciphertext) 
    {
        ChaCha20::encrypt(key, nonce, 1, plaintext, ciphertext);
        return true;
    }

    // Chiều giải mã: Dữ liệu mã hóa -> Dữ liệu nén ban đầu
    static bool decrypt(const std::vector<uint8_t>& ciphertext,
                        const std::array<uint8_t, 32>& key,
                        const std::array<uint8_t, 12>& nonce,
                        std::vector<uint8_t>& plaintext) 
    {
        ChaCha20::encrypt(key, nonce, 1, ciphertext, plaintext);
        return true;
    }
};

#endif // CRYPTO_ENGINE_HPP