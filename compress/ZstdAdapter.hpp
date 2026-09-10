#ifndef ZSTD_ADAPTER_HPP
#define ZSTD_ADAPTER_HPP

#include "ICompressor.hpp"
#include <zstd.h>
#include <iostream>
#include <vector>
#include <stdexcept>

class ZstdAdapter : public ICompressor {
private:
    int compressionLevel; // Mức nén từ 1 (nhanh nhất) đến 19 (nén sâu nhất), mặc định là 3

public:
    explicit ZstdAdapter(int level = 3) : compressionLevel(level) {}

    // Chiều Nén
    bool compress(const std::vector<uint8_t>& input, 
                  std::vector<uint8_t>& output, 
                  uint8_t& padding) override 
    {
        if (input.empty()) return false;
        padding = 0;

        size_t const maxCompressedSize = ZSTD_compressBound(input.size());
        output.resize(maxCompressedSize);

        size_t const cSize = ZSTD_compress(
            output.data(), maxCompressedSize,
            input.data(), input.size(),
            compressionLevel
        );

        if (ZSTD_isError(cSize)) {
            std::cerr << "[Lỗi Zstd Compress]: " << ZSTD_getErrorName(cSize) << "\n";
            return false;
        }

        output.resize(cSize);
        return true;
    }

    bool decompress(const std::vector<uint8_t>& input, 
                    uint64_t originalSize, 
                    std::vector<uint8_t>& output) override 
    {
        if (originalSize == 0) return true;

        output.resize(originalSize);

        // Thực hiện giải nén dữ liệu
        size_t const dSize = ZSTD_decompress(
            output.data(), originalSize,
            input.data(), input.size()
        );

        if (ZSTD_isError(dSize)) {
            std::cerr << "[Lỗi Zstd Decompress]: " << ZSTD_getErrorName(dSize) << "\n";
            return false;
        }

        if (dSize != originalSize) {
            std::cerr << "[Lỗi Zstd]: Kích thước sau giải nén (" << dSize 
                      << ") không khớp originalSize (" << originalSize << ")\n";
            return false;
        }

        return true;
    }
};

#endif // ZSTD_ADAPTER_HPP