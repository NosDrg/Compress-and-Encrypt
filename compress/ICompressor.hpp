#ifndef ICOMPRESSOR_HPP
#define ICOMPRESSOR_HPP

#include <vector>
#include <cstdint>

// Interface for a compressor that defines methods for compressing and decompressing data
class ICompressor {
public:
    virtual ~ICompressor() = default;
    virtual bool compress(const std::vector<uint8_t>& input, 
                          std::vector<uint8_t>& output, 
                          uint8_t& padding) = 0;
    virtual bool decompress(const std::vector<uint8_t>& input, 
                            uint64_t originalSize, 
                            std::vector<uint8_t>& output) = 0;
};

#endif