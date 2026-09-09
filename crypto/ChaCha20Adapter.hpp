#ifndef CHACHA20_ADAPTER_HPP
#define CHACHA20_ADAPTER_HPP

#include "ICipher.hpp"
#include "ChaCha20.hpp"
#include <random>
#include <cstdint>

class ChaCha20Adapter : public ICipher {
public:
    void generateNonce(std::array<uint8_t, 12>& nonce) override {
        std::random_device rd;
        for (int i = 0; i < 12; ++i) {
            nonce[i] = static_cast<uint8_t>(rd() & 0xFF);
        }
    }

    bool encrypt(const std::vector<uint8_t>& input,
                 const std::array<uint8_t,32>& key,
                 const std::array<uint8_t,12>& nonce,
                 std::vector<uint8_t>& output) override 
    {
        ChaCha20::encrypt(key, nonce, 1, input, output);
        return true;
    }

    bool decrypt(const std::vector<uint8_t>& input,
                 const std::array<uint8_t, 32>& key,
                 const std::array<uint8_t, 12>& nonce,
                 std::vector<uint8_t>& output) override 
    {
        ChaCha20::encrypt(key, nonce, 1, input, output);
        return true;
    }
};

#endif // CHACHA20_ADAPTER_HPP