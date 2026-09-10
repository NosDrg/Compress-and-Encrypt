#ifndef HUFFMAN_HPP
#define HUFFMAN_HPP

#include "bitReader.hpp"
#include "bitWriter.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <queue>
#include <array>
#include <iostream>
#include <stdexcept>

// Use Huffman tree to store the compressed data. 
// Each node in the tree represents a character and its corresponding frequency. 
// The left and right pointers are used to traverse the tree during encoding and decoding.
struct HuffmanNode {
    uint8_t data;               // The character represented by this node
    uint64_t freq;              // Total number of bits in the compressed data
    HuffmanNode* left;          // Pointer to the left child node (for tree structures)
    HuffmanNode* right;         // Pointer to the right child node (for tree structures)

    // Constructor to initialize a leaf node with a character and its frequency
    HuffmanNode(uint8_t val, uint64_t frequency) : data(val), freq(frequency), left(nullptr), right(nullptr) {}

    // Constructor to initialize an internal node with a frequency and child nodes
    HuffmanNode(uint64_t frequency, HuffmanNode* l, HuffmanNode* r) : data(0), freq(frequency), left(l), right(r) {}

    // Method to check if the current node is a leaf node (i.e., has no children)
    bool isLeaf() const {
        return (left == nullptr && right == nullptr);
    }
};

struct NodeComparator {
    // Comparator for priority queue to order nodes based on frequency
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        return a->freq > b->freq; // Min-heap based on frequency
    }
};

class HuffmanHandler {
public:
    // Method to compress input data and write it to an output stream
    static inline uint8_t compressData(const std::vector<uint8_t>& inputData, std::ostream& outputStream) {
        HuffmanNode* root = buildHuffmanTree(inputData);

        std::string code_table[256];
        generateHuffmanCodes(root, "", code_table);

        BitWriter bitWriter(outputStream);
        writeCompressedData(root, bitWriter);

        for (uint8_t byte : inputData) {
            const std::string& code = code_table[byte];
            for (char bit : code) {
                bitWriter.writeBit(bit == '1');
            }
        }

        uint8_t remainingBits = bitWriter.flush();
        deleteHuffmanTree(root);
        return remainingBits;
    }

    // Method to decompress data from an input stream and return the original data
    static inline std::vector<uint8_t> decompressData(std::istream& inputStream, uint64_t originalSize) {
        std::vector<uint8_t> outputData;
        if (originalSize == 0) return outputData;

        outputData.reserve(originalSize);
        BitReader bitReader(inputStream);

        HuffmanNode* root = readCompressedData(bitReader);
        if (!root) {
            throw std::runtime_error("Failed to read Huffman tree from the input stream.");
        }

        if (root->isLeaf()) {
            outputData.assign(originalSize, root->data);
            deleteHuffmanTree(root);
            return outputData;
        }

        HuffmanNode* currentNode = root;
        while (outputData.size() < originalSize) {
            int bit = bitReader.readBit();
            if (bit == -1) break;

            currentNode = (bit == 0) ? currentNode->left : currentNode->right;
            if (!currentNode) {
                throw std::runtime_error("Invalid bit sequence encountered during decompression.");
            }

            if (currentNode->isLeaf()) {
                outputData.push_back(currentNode->data);
                currentNode = root;
            }
        }

        deleteHuffmanTree(root);
        return outputData;
    }

private:
    static inline HuffmanNode* buildHuffmanTree(const std::vector<uint8_t>& inputData) {
        std::array<uint64_t, 256> frequencyMap = {0};
        for (uint8_t byte : inputData) {
            frequencyMap[byte]++;
        }

        std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, NodeComparator> minHeap;
        for (int i = 0; i < 256; ++i) {
            if (frequencyMap[i] > 0) {
                minHeap.push(new HuffmanNode(i, frequencyMap[i]));
            }
        }

        while (minHeap.size() > 1) {
            HuffmanNode* left = minHeap.top(); minHeap.pop();
            HuffmanNode* right = minHeap.top(); minHeap.pop();

            uint64_t combinedFreq = left->freq + right->freq;
            HuffmanNode* internalNode = new HuffmanNode(combinedFreq, left, right);
            minHeap.push(internalNode);
        }

        return minHeap.empty() ? nullptr : minHeap.top();
    }

    static inline void generateHuffmanCodes(HuffmanNode* root, const std::string& code, std::string code_table[256]) {
        if (!root) return;
        if (root->isLeaf()) {
            code_table[root->data] = code;
            return;
        }
        generateHuffmanCodes(root->left, code + "0", code_table);
        generateHuffmanCodes(root->right, code + "1", code_table);
    }

    static inline void writeCompressedData(const HuffmanNode* root, BitWriter& bitWriter) {
        if (!root) return;
        if (root->isLeaf()) {
            bitWriter.writeBit(1);
            bitWriter.writeBits(root->data, 8);
            return;
        }
        bitWriter.writeBit(0);
        writeCompressedData(root->left, bitWriter);
        writeCompressedData(root->right, bitWriter);
    }

    static inline HuffmanNode* readCompressedData(BitReader& bitReader) {
        int bit = bitReader.readBit();
        if (bit == -1) {
            std::cerr << "Error: Unexpected end of stream while reading compressed data." << std::endl;
            return nullptr;
        }

        if (bit == 1) {
            uint8_t character;
            if (!bitReader.readBits(character)) {
                std::cerr << "Error: Unexpected end of stream while reading character." << std::endl;
                return nullptr;
            }
            return new HuffmanNode(character, 0);
        }

        HuffmanNode* leftChild = readCompressedData(bitReader);
        HuffmanNode* rightChild = readCompressedData(bitReader);
        return new HuffmanNode(0, leftChild, rightChild);
    }

    static inline void deleteHuffmanTree(HuffmanNode* root) {
        if (!root) return;
        deleteHuffmanTree(root->left);
        deleteHuffmanTree(root->right);
        delete root;
    }
};

#endif // HUFFMAN_HPP