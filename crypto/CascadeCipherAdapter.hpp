#ifndef CASCADE_CIPHER_ADAPTER_HPP
#define CASCADE_CIPHER_ADAPTER_HPP

#include "ICipher.hpp"
#include "Aes256Adapter.hpp"
#include "ChaCha20Adapter.hpp"
#include <memory>
#include <cstring>

class CascadeCipherAdapter : public ICipher {
private:
    Aes256Adapter aesCipher;
    ChaCha20Adapter chachaCipher;

    // Splits a 32-byte main key into 2 independent 32-byte subkeys
    void deriveSubKeys(const std::array<uint8_t, 32>& masterKey,
                       std::array<uint8_t, 32>& aesKey,
                       std::array<uint8_t, 32>& chachaKey) 
    {
        for (size_t i = 0; i < 32; ++i) {
            aesKey[i] = masterKey[i] ^ 0xAA;    // Domain separator 1
            chachaKey[i] = masterKey[i] ^ 0x55; // Domain separator 2
        }
    }

public:
    void generateNonce(std::array<uint8_t, 12>& nonce) override {
        aesCipher.generateNonce(nonce);
    }

    bool encrypt(const std::vector<uint8_t>& input,
                 const std::array<uint8_t, 32>& key,
                 const std::array<uint8_t, 12>& nonce,
                 std::vector<uint8_t>& output,
                 std::array<uint8_t, 16>& outTag) override 
    {
        std::array<uint8_t, 32> aesKey, chachaKey;
        deriveSubKeys(key, aesKey, chachaKey);

        // Generate 2 separate Nonces from the original Nonce
        std::array<uint8_t, 12> nonceAes = nonce;
        std::array<uint8_t, 12> nonceChaCha = nonce;
        nonceChaCha[0] ^= 0xFF; // Đảm bảo nonce 2 tầng không bao giờ trùng nhau

        // Layer 1: AES-256-GCM encryption
        std::vector<uint8_t> intermediateCiphertext;
        std::array<uint8_t, 16> tagAes;
        if (!aesCipher.encrypt(input, aesKey, nonceAes, intermediateCiphertext, tagAes)) {
            return false;
        }

        // Layer 2: Encrypt ChaCha20-Poly1305 on Layer 1 results
        std::array<uint8_t, 16> tagChaCha;
        if (!chachaCipher.encrypt(intermediateCiphertext, chachaKey, nonceChaCha, output, tagChaCha)) {
            return false;
        }

        // Combine 2 Tags (XOR or chain) into 1 representative Tag
        for (int i = 0; i < 16; ++i) {
            outTag[i] = tagAes[i] ^ tagChaCha[i];
        }

        return true;
    }

    bool decrypt(const std::vector<uint8_t>& input,
                 const std::array<uint8_t, 32>& key,
                 const std::array<uint8_t, 12>& nonce,
                 const std::array<uint8_t, 16>& expectedTag,
                 std::vector<uint8_t>& output) override 
    {
        std::array<uint8_t, 32> aesKey, chachaKey;
        deriveSubKeys(key, aesKey, chachaKey);

        std::array<uint8_t, 12> nonceAes = nonce;
        std::array<uint8_t, 12> nonceChaCha = nonce;
        nonceChaCha[0] ^= 0xFF;

        // Decoding direction: Layer 2 decoding first (ChaCha20)
        std::vector<uint8_t> intermediateCiphertext;
        if (!chachaCipher.decrypt(input, chachaKey, nonceChaCha, expectedTag, intermediateCiphertext)) {
            return false;
        }

        // Then decrypt Layer 1 (AES-256)
        if (!aesCipher.decrypt(intermediateCiphertext, aesKey, nonceAes, expectedTag, output)) {
            return false;
        }

        return true;
    }
};

#endif // CASCADE_CIPHER_ADAPTER_HPP