#ifndef HUFFMAN_ADAPTER_HPP
#define HUFFMAN_ADAPTER_HPP

#include "ICompressor.hpp"
#include "Huffman.hpp"
#include <sstream>

class HuffmanAdapter : public ICompressor {
public:
    
    bool compress(const std::vector<uint8_t>& input, 
                    std::vector<uint8_t>& output,
                    uint8_t& padding) override
    {
        if (input.empty()) return false;
        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        padding = HuffmanHandler::compressData(input, ss);
        std::string s = ss.str();
        output.assign(s.begin(), s.end());
        return true;
    }

    bool decompress(const std::vector<uint8_t>& input, 
                    uint64_t originalSize, 
                    std::vector<uint8_t>& output) override 
    {
        if (originalSize == 0) return true;
        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        ss.write(reinterpret_cast<const char*>(input.data()), input.size());
        ss.seekg(0);
        output = HuffmanHandler::decompressData(ss, originalSize);
        return output.size() == originalSize;
    }

};

#endif  // HUFFMAN_ADAPTER_HPP