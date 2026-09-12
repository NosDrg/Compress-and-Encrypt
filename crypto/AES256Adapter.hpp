#ifndef AES256_ADAPTER_HPP
#define AES256_ADAPTER_HPP

#include "ICipher.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iostream>
#include <vector>
#include <array>
#include <stdexcept>

class Aes256Adapter : public ICipher {
public:
    void generateNonce(std::array<uint8_t, 12>& nonce) override {
        if (RAND_bytes(nonce.data(), 12) != 1) {
            throw std::runtime_error("[Error AES] Can't generate random Nonce from OpenSSL.");
        }
    }

    bool encrypt(const std::vector<uint8_t>& input,
                const std::array<uint8_t, 32>& key,
                const std::array<uint8_t, 12>& nonce,
                std::vector<uint8_t>& output,
                std::array<uint8_t, 16>& outTag) override 
    {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return false;

        bool success = false;
        int len = 0;
        int ciphertext_len = 0;

        output.resize(input.size());

        // 1. Initialize the AES-256-GCM encryption process
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) goto cleanup;

        // 2. Set IV / Nonce length (GCM standard 12 bytes)
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, NULL) != 1) goto cleanup;

        // 3. Load Key and Nonce
        if (EVP_EncryptInit_ex(ctx, NULL, NULL, key.data(), nonce.data()) != 1) goto cleanup;

        // 4. Perform data encryption
        if (EVP_EncryptUpdate(ctx, output.data(), &len, input.data(), input.size()) != 1) goto cleanup;
        ciphertext_len = len;

        // 5. Finish the encryption process
        if (EVP_EncryptFinal_ex(ctx, output.data() + len, &len) != 1) goto cleanup;
        ciphertext_len += len;
        output.resize(ciphertext_len);

        // 6. Get the Authentication Tag (16 bytes)
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, outTag.data()) != 1) goto cleanup;

        success = true;

    cleanup:
        EVP_CIPHER_CTX_free(ctx);
        return success;
    }

    bool decrypt(const std::vector<uint8_t>& input,
                const std::array<uint8_t, 32>& key,
                const std::array<uint8_t, 12>& nonce,
                const std::array<uint8_t, 16>& expectedTag,
                std::vector<uint8_t>& output) override 
    {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return false;

        bool success = false;
        int len = 0;
        int plaintext_len = 0;

        output.resize(input.size());

        // 1. Initialize the AES-256-GCM decryption process
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) goto cleanup;

        // 2. Set IV/Nonce length
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, NULL) != 1) goto cleanup;

        // 3. Load Key and Nonce
        if (EVP_DecryptInit_ex(ctx, NULL, NULL, key.data(), nonce.data()) != 1) goto cleanup;

        // 4. Decode data
        if (EVP_DecryptUpdate(ctx, output.data(), &len, input.data(), input.size()) != 1) goto cleanup;
        plaintext_len = len;

        // 5. Assign the Expected Tag to the context so that OpenSSL can check the integrity
        // Cast const_cast because the old OpenSSL API requires a non-const pointer for the input tag
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, const_cast<uint8_t*>(expectedTag.data())) != 1) {
            std::cerr << "[AES Error] Unable to set Authentication Tag.\n";
            goto cleanup;
        }

        // 6. Perform Tag authentication and finish decoding.
        if (EVP_DecryptFinal_ex(ctx, output.data() + len, &len) <= 0) {
            std::cerr << "[AEAD Error - AES-256] Tag authentication failed! The data is tampered with or the password is incorrect.\n";
            goto cleanup;
        }

        plaintext_len += len;
        output.resize(plaintext_len);
        success = true;

    cleanup:
        EVP_CIPHER_CTX_free(ctx);
        return success;
    }
};

#endif // AES256_ADAPTER_HPP