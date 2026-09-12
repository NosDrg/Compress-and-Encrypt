#ifndef CHACHA20_ADAPTER_HPP
#define CHACHA20_ADAPTER_HPP

#include "ICipher.hpp"
#include "ChaCha20.hpp"
#include "Poly1305.hpp"
#include <random>
#include <cstring>
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
                const std::array<uint8_t, 32>& key,
                const std::array<uint8_t, 12>& nonce,
                std::vector<uint8_t>& output,
                std::array<uint8_t, 16>& outTag) override 
    {
        // 1. Sinh Poly1305 One-Time Key bằng khối ChaCha20 đầu tiên (counter = 0)
        ChaCha20 keyGen;
        keyGen.initialize(key, nonce, 0);
        uint8_t firstBlock[64];
        keyGen.generateKeystreamBlock(firstBlock);

        uint8_t polyKey[32];
        std::memcpy(polyKey, firstBlock, 32); // Lấy 32 bytes đầu

        // 2. Mã hóa dữ liệu bằng ChaCha20 bắt đầu từ counter = 1
        ChaCha20::encrypt(key, nonce, 1, input, output);

        // 3. Tính toán Authentication Tag 16 bytes bằng Poly1305
        Poly1305::computeTag(polyKey, output, outTag);
        return true;
    }

    bool decrypt(const std::vector<uint8_t>& input,
                const std::array<uint8_t, 32>& key,
                const std::array<uint8_t, 12>& nonce,
                const std::array<uint8_t, 16>& expectedTag,
                std::vector<uint8_t>& output) override 
    {
        // 1. Tái tạo Poly1305 Key từ counter = 0
        ChaCha20 keyGen;
        keyGen.initialize(key, nonce, 0);
        uint8_t firstBlock[64];
        keyGen.generateKeystreamBlock(firstBlock);

        uint8_t polyKey[32];
        std::memcpy(polyKey, firstBlock, 32);

        // 2. Xác thực thẻ Tag trước khi giải mã
        std::array<uint8_t,16> calculatedTag;
        Poly1305::computeTag(polyKey, input, calculatedTag);

        // So sánh theo thời gian hằng định (Constant-Time Compare) chống Timing Attack
        int diff = 0;
        for (int i = 0; i < 16; ++i) {
            diff |= (calculatedTag[i] ^ expectedTag[i]);
        }

        if (diff != 0) {
            std::cerr << "[Lỗi AEAD] Authentication Tag không khớp! Dữ liệu bị giả mạo hoặc sai mật khẩu.\n";
            return false;
        }

        // 3. Nếu Tag hợp lệ, tiến hành giải mã
        ChaCha20::encrypt(key, nonce, 1, input, output);
        return true;
    }
};

#endif // CHACHA20_ADAPTER_HPP